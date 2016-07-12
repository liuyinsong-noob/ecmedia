package com.hisun.phone.core.voice.listener;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.Device.CallType;
import com.hisun.phone.core.voice.DeviceListener.Reason;

/**
 * 
* <p>Title: VoIPListener.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-22
* @version 3.5
 */
public interface OnVoIPListener {

	/**
	 * 回调客户端正在呼叫中
	 * @param callid 当前通话的唯一标识
	 * 
	 * @see Device#makeCall(CallType, String)
	 */
	public abstract void onCallProceeding(String callid);

	/**
	 * 回调客户端对方正在振铃
	 * @param callid 当前通话的唯一标识
	 * 
	 * @see Device#makeCall(CallType, String)
	 */
	public abstract void onCallAlerting(String callid);

	/**
	 * 回调客户端，通话结束
	 * @param callid 当前通话的唯一标识
	 * 
	 * @see Device#releaseCall(String)
	 */
	public abstract void onCallReleased(String callid);

	/**
	 * 回调客户端，对方应答（按下接听按键）
	 * @param callid 当前通话的唯一标识
	 * 
	 * @see Device#acceptCall(String, CallType)
	 */
	public abstract void onCallAnswered(String callid);

	/**
	 * 回调客户端，当前通话被挂机（暂停），仅作用于电话挂起主调方
	 * 通话另一方将收到{@link #onCallPausedByRemote(String)}通知
	 * 
	 * @param callid 当前通话的唯一标识
	 * 
	 * @see Device#pauseCall(String)
	 */
	public abstract void onCallPaused(String callid);

	/**
	 * 通知客户端，当前通话被远端挂机（暂停），仅作用于电话挂起非主调方
	 * @param callid 当前通话的唯一标识
	 * 
	 * @see Device#pauseCall(String)
	 */
	public abstract void onCallPausedByRemote(String callid);

	/**
	 * 回调通话呼叫发起方，由于某个原因呼叫发起失败
	 * 如:通过{@link Reason#NOTFOUND#getStatus()} 获取错误代码
	 * {@link Reason#NOTFOUND#getValue()} 获取错误描述
	 * 
	 * @param callid 当前通话的唯一标识
	 * @param reason 呼叫失败原因
	 * 
	 * @see Device#makeCall(CallType, String)
	 */
	public abstract void onMakeCallFailed(String callid, Reason reason);

	/**
	 * 通话过程中，做了呼叫转移，转移成功后，通过这个接口通知呼叫转移发起方。
	 * @param callid 当前通话的唯一标识
	 * @param destination 转接的号码
	 */
	public abstract void onCallTransfered(String callid, String destination);


	/**
	 * 回调通知回拨发起方，当前回拨状态,reason为0代表回拨呼叫成功，其他则失败
	 * 
	 * @param reason 呼叫失败原因
	 * @param src 呼叫方号码
	 * @param dest 被叫方号码
	 */
	public abstract void onCallback(int reason, String src, String dest);

	/**
	 * @see #onSwitchCallMediaTypeRequest(String, int)
	 * @param callid
	 * @param reason
	 */
	@Deprecated void onCallMediaUpdateRequest(String callid, int reason);
	/**
	 * 收到对方请求切换音视频的消息
	 * @param callid 当前通话的唯一标识
	 * @param callType {@link CallType#VOICE}代表请求增加视频,
	 * 		     需要调用{@link Device#responseSwitchCallMediaType(String, int)}接口进行响应.
	 * 		  {@link CallType#VIDEO}代表请求删除视频，不需要响应。
	 * 
	 * @see Device#requestSwitchCallMediaType(String, CallType)
	 */
	void onSwitchCallMediaTypeRequest(String callid, CallType callType);

	/**
	 * @see #onSwitchCallMediaTypeResponse(String, int)
	 * @param callid
	 * @param reason
	 */
	@Deprecated void onCallMediaUpdateResponse(String callid, int reason);
	
	/**
	 * 收到对方回复切换音视频请求的应答
	 * 即对方是否同意了切换音视频请求，该接口主要的功能是通知调用切换音视频请求后的结果，本地不用另外做处理。
	 * @param callid 当前通话的唯一标识
	 * @param callType 更新后的媒体状态：CallType#VIDEO}表示视频
	 * 
	 * @see Device#responseSwitchCallMediaType(String, int)
	 */
	void onSwitchCallMediaTypeResponse(String callid, CallType callType);

	/**
	 * 视频分辨率改变，呼叫过程中，对方视频的分辨率发生改变，通过该接口通知对方
	 * @param callid 当前通话的唯一标识
	 * @param resolution 当前改变的视频分辨率（a x b）
	 * 
	 */
	 @Deprecated public void onCallVideoRatioChanged(String callid, String resolution);
	 void onCallVideoRatioChanged(String callid, int width , int height);

	/**
	 * @see #onCallMediaInitFailed(String, CallType)
	 * @param callid
	 * @param reason
	 */
	@Deprecated void onCallMediaInitFailed(String callid, int reason);
	/**
	 * 媒体初始化失败 
	 * @param callid 当前通话的唯一标识
	 * @param callType 表明是音频还是视频
	 */
	void onCallMediaInitFailed(String callid, CallType callType);
	
	/**
	 * 呼转成功回调接口
	 * @param callid 当前通话的唯一标识
	 * @param result 转接结果 0代表成功，其他则失败
	 */
	void onTransferStateSucceed(String callid , boolean result);
	
	/**
	 * 通话录音过程通知回调
	 */
	public interface OnCallRecordListener {
		
		/**
		 * 通话录音结束
		 * @param filePath 录音文件本地全路径
		 */
		void onCallRecordDone(String filePath);
		
		/**
		 * 通话录音失败
		 * @param reason 0成功，-1录音失败并删除录音文件，-2录音写文件失败保留录音文件
		 */
		void onCallRecordError(int reason);
	}
	
	/**
	 * Interface definition for a callback to be invoked when enabled that 
     * want to process audio data.
	 */
	public interface OnCallProcessDataListener {
		
		/**
		 * Called when a flag has be set by @setProcessDataEnabled 
		 * 
		 * @param b The audio data
		 * @param transDirection Transmitting or receiving the data transfer direction
		 * @return  The audio data after the operation (Example: Encryption) 
		 */
		byte[] onCallProcessData(byte[] b , int transDirection);
	}
}


