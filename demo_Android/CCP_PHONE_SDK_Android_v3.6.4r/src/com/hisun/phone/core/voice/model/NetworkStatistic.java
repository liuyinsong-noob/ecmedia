package com.hisun.phone.core.voice.model;

/**
 * 
 * @ClassName: NetworkStatistic.java
 * @Description: Class that provides network traffic statistics.
 * @author 云通讯
 * @date 2014-2-14
 * @version 3.6.1
 */
public class NetworkStatistic {

	/**
	 * 媒体交互的持续时间，单位秒，可能为0；
	 */
	private long duration;
	
	/**
	 * 在duration时间内，网络发送的总流量，单位字节；
	 */
	private long txBytes;
	
	/**
	 * 在duration时间内，网络接收的总流量，单位字节；
	 */
	private long rxBytes;

	
	/**
	 * 
	 */
	public NetworkStatistic() {
		super();
	}

	/**
	 * @param duration
	 * @param txBytes
	 * @param rxBytes
	 */
	public NetworkStatistic(long duration, long txBytes, long rxBytes) {
		super();
		this.duration = duration;
		this.txBytes = txBytes;
		this.rxBytes = rxBytes;
	}

	public long getDuration() {
		return duration;
	}

	/**
	 * 在duration时间内，网络接收的总流量，单位字节；
	 * @return
	 */
	public long getTxBytes() {
		return txBytes;
	}
	
	/**
	 * 在duration时间内，网络接收的总流量，单位字节；
	 * @return
	 */
	public long getRxBytes() {
		return rxBytes;
	}

	
}
