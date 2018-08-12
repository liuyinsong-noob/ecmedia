/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.hisun.phone.core.voice.util;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;

import android.util.Log;

/**
 * 
 */
public class UDPSocketUtil {
	public static final String TAG = UDPSocketUtil.class.getSimpleName();
	public static final int INTERVAL = 20; 
	private int timeout = 1 * 60 * 1000;
	private DatagramSocket socket;
	
	private boolean isReceiverStop = false;
	private boolean isSenderStop = false;
	private int localPort = 0;
	
	private Thread senderThread;
	private Thread receiverThread;
	
	public InetAddress ServerAddress;
	public int ServerPort;
	private final ConcurrentHashMap<String, Long> sendPacketMap = new ConcurrentHashMap<String, Long>();
	
	private int sendPacketCount = 0;
	private int recePacketCount = 0;
	
	private long minDelay = Short.MAX_VALUE;
	private long maxDelay = 0;
	private long sumDelay = 0;
	
	private long starttime = 0;
	private long endtime = 0;
	
	public UDPSocketUtil() {
		try {
			ServerAddress = InetAddress.getByName("42.121.115.160");
			ServerPort = 2009;
		} catch (UnknownHostException e) {
			e.printStackTrace();
		}
		init();
	}

	private void init() {
		try {
			if (socket == null) {
				socket = new DatagramSocket();
				localPort = socket.getLocalPort();
				Log.w(TAG, getFullTag("LocalPort") + localPort);
				socket.setSoTimeout(timeout);
				printSocket();
			} else {
				Log.w(TAG, "Socket already init.");
			}
		} catch (SocketException se) {
			se.printStackTrace();
		}
		
		sendPacketCount = 0;
		recePacketCount = 0;
		
		minDelay = Short.MAX_VALUE;
		maxDelay = 0;
		sumDelay = 0;
		starttime = 0;
		endtime = 0;
	}
	
	/**
	 * init work thread
	 */
	private void initWorkThread() {
		if (receiverThread == null) {
			receiverThread = new Thread(mReceiverRunnable);
		}
		receiverThread.start();
		
		if (senderThread == null) {
			senderThread = new Thread(mSenderRunnable);
		}
		senderThread.start();
	}
	
	public void start() {
		init();
		isReceiverStop = false;
		isSenderStop = false;
		
		initWorkThread();
		starttime = System.currentTimeMillis();
	}
	
	public void stop() {
		try {
			isReceiverStop = true;
			isSenderStop = true;
			
			try {
				Thread.sleep(500L);
			} catch (InterruptedException e) {
			}
			
			if (senderThread != null && senderThread.isAlive()) {
				senderThread.interrupt();
			}

			if (receiverThread != null && receiverThread.isAlive()) {
				receiverThread.interrupt();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		senderThread = null;
		receiverThread = null;
		
		endtime = System.currentTimeMillis();
		
		destroy();
	}
	
	private final byte [] sendBuffer = new byte[0];
	private final DatagramPacket dataPacket = new DatagramPacket(sendBuffer, 0);
	public static final SimpleDateFormat sequenceFormat = new SimpleDateFormat("yyyyMMddHHmmss");
	public static final String HOLDPLACE = nextHexString(49);
	public static int K = 1;
	public static final long seed = System.currentTimeMillis();
	
	/** Returns a random integer between 0 and n-1 */
	static int nextInt(int n) {
		Random rand = new Random(seed);
		return Math.abs(rand.nextInt()) % n;
	}
	
	/** Returns a random hexadecimal String */
	static String nextHexString(int len) {
		byte[] buff = new byte[len];
		for (int i = 0; i < len; i++) {
			int n = nextInt(16);
			buff[i] = (byte) ((n < 10) ? 48 + n : 87 + n);
		}
		return new String(buff);
	}
	
	static String getSequenceFormat(long t) {
		//long t = System.currentTimeMillis();
		Date d = new Date(t);
		String date = sequenceFormat.format(d);
		
		return date + "$" + (K++) + "%" + HOLDPLACE + "@" + t;
	}
	
	private Runnable mSenderRunnable = new Runnable() {
		@Override
		public void run() {
			while (!isSenderStop && sendPacketCount < 1000) {
				try {
					long t = System.currentTimeMillis();
					final String sendPacket = getSequenceFormat(t);
					if (sendPacket == null) {
						continue;
					}

					final String messageid = sendPacket;
					sendPacketMap.put(messageid, t);
					Log.w(TAG, getFullTag("SenderRunnable") + "get send packet: \r\n" + messageid + "\r\n");
					
					byte[] buffer = messageid.getBytes();
					dataPacket.setData(buffer);
					dataPacket.setAddress(ServerAddress);
					dataPacket.setPort(ServerPort);
					
					socket.send(dataPacket);
					//sendPacketCount++;
					Log.i(TAG, getFullTag("SenderRunnable") + "send over.");
					
					// interval time 
					try {
						Thread.sleep(1 * INTERVAL);
					} catch (InterruptedException e) {
					}
				} catch (Throwable th) {
					th.printStackTrace();
				} 
				sendPacketCount++;
			}
			if(sendPacketCount == 1000 ) {
				stop();
				if(mListener != null ) {
					mListener.onSocketLayerComplete();
				}
			}
		}
	};
	
	private boolean isExistInSendMapping(final String packetKey) {
		if (sendPacketMap == null || packetKey == null) {
			return false;
		}
		if (sendPacketMap.containsKey(packetKey) && sendPacketMap.get(packetKey) != null) {
			long value = sendPacketMap.get(packetKey);
			long dt = System.currentTimeMillis() - value;
			
			sumDelay += dt;
			
			if (dt < minDelay) {
				minDelay = dt;
			}

			if (dt > maxDelay) {
				maxDelay = dt;
			}
			
			return true;
		}
		return false;
	}
	
	private void remove(String messageid) {
		sendPacketMap.remove(messageid);
	}
	
	private void receive(byte [] data) {
		try {
			final String msgid = new String(data, 0, data.length);
			Log.i(TAG, getFullTag("receive") + "received content: \r\n" + msgid + "\r\n");
			if (isExistInSendMapping(msgid)) {
				remove(msgid);
				Log.i(TAG, getFullTag("receive") + "remove key <" + msgid + "> from <sendPacketMap>.");
			} else {
				Log.w(TAG, getFullTag("receive") + "sendPacketMap key <" + msgid + "> not exist.");
			}
		} catch (Exception e) {
			e.printStackTrace();
			Log.e(TAG, e.toString());
		}
		
	}
	
	private final byte[] receiveBuffer = new byte[4 * 1024];
	
	private Runnable mReceiverRunnable = new Runnable () {

		@Override
		public void run() {
			while (!isReceiverStop) {
				try {
					// adapter 4.0 phone
					DatagramPacket receivePacket = new DatagramPacket(receiveBuffer, receiveBuffer.length, Inet4Address.getLocalHost(), localPort);
					// This method blocks until a packet is received or a timeout has expired.
					socket.receive(receivePacket);
					int receiveLength = receivePacket.getLength();
					Log.w(TAG, getFullTag("ReceiverRunnable") + "received " + receiveLength + " bytes data from server.");
					
					if (receiveLength > 0) {
						recePacketCount++;
						byte[] b = new byte[receiveLength];
						System.arraycopy(receiveBuffer, 0, b, 0, receiveLength);
						byte[] data = b.clone();
						receive(data);
					}
				} catch(SocketTimeoutException ste){
					Log.w(TAG, getFullTag("ReceiverRunnable") + "udp receiver thread wait " + timeout + "ms timeout.");
					try {
						Thread.sleep(20L);
					} catch (InterruptedException e) {
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		}
		
	};
	
	void printSocket(){
		if(socket != null) {
			Log.e(TAG, "--------------------------------------------------------------------");
			Log.w(TAG, "Socket - LocalPort: " + socket.getLocalPort());
			Log.w(TAG, "Socket - RemotePort: " + socket.getPort());
			Log.w(TAG, "Socket - isBind: " + socket.isBound());
			Log.w(TAG, "Socket - isClosed: " + socket.isClosed());
			Log.w(TAG, "Socket - isConnected: " + socket.isConnected());
			Log.e(TAG, "--------------------------------------------------------------------");
		}
	}

	private String getFullTag(String lastName) {
		return "[" + lastName + "] ";
	}
	
	private void destroy(){
		if (socket != null) {
			socket.disconnect();
			socket.close();
			socket = null;
		}
		
		sendPacketMap.clear();
		mListener = null;
	}

	/**
	 * @return the minDelay
	 */
	public long getMinDelay() {
		return minDelay;
	}

	/**
	 * @param minDelay the minDelay to set
	 */
	public void setMinDelay(long minDelay) {
		this.minDelay = minDelay;
	}

	/**
	 * @return the maxDelay
	 */
	public long getMaxDelay() {
		return maxDelay;
	}

	/**
	 * @param maxDelay the maxDelay to set
	 */
	public void setMaxDelay(long maxDelay) {
		this.maxDelay = maxDelay;
	}

	/**
	 * @return the starttime
	 */
	public long getStarttime() {
		return starttime;
	}

	/**
	 * @param starttime the starttime to set
	 */
	public void setStarttime(long starttime) {
		this.starttime = starttime;
	}

	/**
	 * @return the endtime
	 */
	public long getEndtime() {
		return endtime;
	}

	/**
	 * @param endtime the endtime to set
	 */
	public void setEndtime(long endtime) {
		this.endtime = endtime;
	}

	/**
	 * @return the sendPacketCount
	 */
	public int getSendPacketCount() {
		return sendPacketCount;
	}

	/**
	 * @param sendPacketCount the sendPacketCount to set
	 */
	public void setSendPacketCount(int sendPacketCount) {
		this.sendPacketCount = sendPacketCount;
	}

	/**
	 * @return the recePacketCount
	 */
	public int getRecePacketCount() {
		return recePacketCount;
	}

	/**
	 * @param recePacketCount the recePacketCount to set
	 */
	public void setRecePacketCount(int recePacketCount) {
		this.recePacketCount = recePacketCount;
	}

	/**
	 * @return the sumDelay
	 */
	public long getSumDelay() {
		return sumDelay;
	}

	/**
	 * @param sumDelay the sumDelay to set
	 */
	public void setSumDelay(long sumDelay) {
		this.sumDelay = sumDelay;
	}
	
	private onSocketLayerListener mListener;
	
	public void setonSocketLayerListener(onSocketLayerListener listener) {
		this.mListener = listener;
	}
	
	public static interface onSocketLayerListener {
		void onSocketLayerComplete();
	}
}
