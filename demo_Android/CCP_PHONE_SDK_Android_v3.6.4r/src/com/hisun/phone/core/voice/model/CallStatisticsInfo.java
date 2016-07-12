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
 */package com.hisun.phone.core.voice.model;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.CallType;

 /**
  * <p>通话数据统计参数信息，可以通过接口{@link Device#getCallStatistics(CallType)}
  * 获得，包含当前通话数据包延迟和丢包率等信息</p>
  * @author yuntongxun.com
  * @date 2014-8-20
  * @version 3.6.3
  */
public class CallStatisticsInfo extends Response{
	private static final long serialVersionUID = -7725201660661796089L;
	private int fractionLost;  		
    private int cumulativeLost;  	
    private int extendedMax;   		
    private int jitterSamples;   	
    private int rttMs;   			
    private int bytesSent;  
    private int packetsSent;  
    private int bytesReceived;  
    private int packetsReceived; 	
    
    /**
     * 上次调用获取统计后这一段时间的丢包率，范围是0~255，255是100%丢失。
     * @return
     */
	public int getFractionLost() {
		return fractionLost;
	}
	public void setFractionLost(int fractionLost) {
		this.fractionLost = fractionLost;
	}
	
	/**
	 * 开始通话后的所有的丢包总个数
	 * @return 丢包总个数
	 */
	public int getCumulativeLost() {
		return cumulativeLost;
	}
	public void setCumulativeLost(int cumulativeLost) {
		this.cumulativeLost = cumulativeLost;
	}
	
	/**
	 * 开始通话后应该收到的包总个数
	 * @return
	 */
	public int getExtendedMax() {
		return extendedMax;
	}
	public void setExtendedMax(int extendedMax) {
		this.extendedMax = extendedMax;
	}
	public int getJitterSamples() {
		return jitterSamples;
	}
	public void setJitterSamples(int jitterSamples) {
		this.jitterSamples = jitterSamples;
	}
	
	/**
	 * 延迟时间，单位是ms
	 */
	public int getRttMs() {
		return rttMs;
	}
	public void setRttMs(int rttMs) {
		this.rttMs = rttMs;
	}
	
	/**
	 * 开始通话后发送的总字节数
	 * @return
	 */
	public int getBytesSent() {
		return bytesSent;
	}
	public void setBytesSent(int bytesSent) {
		this.bytesSent = bytesSent;
	}
	
	/**
	 * 开始通话后发送的总RTP包个数
	 * @return
	 */
	public int getPacketsSent() {
		return packetsSent;
	}
	public void setPacketsSent(int packetsSent) {
		this.packetsSent = packetsSent;
	}
	
	/**
	 * 开始通话后收到的总字节数
	 * @return
	 */
	public int getBytesReceived() {
		return bytesReceived;
	}
	public void setBytesReceived(int bytesReceived) {
		this.bytesReceived = bytesReceived;
	}
	
	/**
	 * 开始通话后收到的总RTP包个数
	 * @return
	 */
	public int getPacketsReceived() {
		return packetsReceived;
	}
	public void setPacketsReceived(int packetsReceived) {
		this.packetsReceived = packetsReceived;
	}
    
}
