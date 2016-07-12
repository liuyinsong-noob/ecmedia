package com.hisun.phone.core.voice.listener;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.CloopenReason;
import com.hisun.phone.core.voice.model.im.IMAttachedMsg;
import com.hisun.phone.core.voice.model.im.IMTextMsg;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
/**
 * 
* <p>Title: OnIMListener.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-10-22
* @version 3.5
 */
public interface OnIMListener {

	/**********************************************************************
	 * MediaMessage *
	 **********************************************************************/

	/**
	 * 录制IM语音消息时间超过限制回调通知接口，当调用SDK接口{@link Device#startVoiceRecording(String, String, boolean, String)}
	 * 发送IM语音消息，SDK会根据当前录制的文件是否超过60s最大长度，超过则会回调该接口通知APP
	 * @param ms 当前录音文件的时长
	 * 
	 * @see Device#startVoiceRecording(String, String, boolean, String)
	 */
	public abstract void onRecordingTimeOut(long ms);


	/**
	 * 当使用SDK录制IM语音消息接口时，SDK会通过该接口回调通知App 当前音量的振幅.
	 * @param amplitude 当前录音振幅（范围在0~100之间不等）
	 * 
	 * @see Device#startVoiceRecording(String, String, boolean, String)
	 */
	public abstract void onRecordingAmplitude(double amplitude);

	/**
	 * SDK完成播放音频文件回调
	 */
	public abstract void onFinishedPlaying();
	
	/**
	 * SDK发送IM消息结果回调，客户端调用SDK接口{@link Device#sendInstanceMessage(String, String, String, String)}
	 * 发送文本和附件消息，SDK执行完发送请求后会通过该接口将发送结果信息回调给接口调用者
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param data 当前发送的消息
	 * 
	 * @see Device#sendInstanceMessage(String, String, String, String)
	 * @see Device#startVoiceRecording(String, String, boolean, String)
	 */
	public abstract void onSendInstanceMessage(CloopenReason reason, InstanceMsg data);
	
	/**
	 * SDK下载IM消息附件回调接口，客户端需要调用SDk接口{@link Device#downloadAttached(java.util.ArrayList)}通知SDK需要下载的参数
	 * SDK完成下载后会通过该接口回调通知客户端下载结果
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * @param fileName 下载文件对应得本地全路径
	 */
	public abstract void onDownloadAttached(CloopenReason reason, String fileName);
	
	/**
	 * 客户端接收新消息事件通知接口，当SDK收到新的IM消息会通过该接口回调通知客户端，参数为新消息的类型
	 * 可以是文本消息、附件消息（语音、图片、文件等）
	 * 
	 * @param message 新消息类型
	 * @see IMTextMsg
	 * @see IMAttachedMsg
	 */
	public abstract void onReceiveInstanceMessage(InstanceMsg message);
	
	/**
	 * 确认消息已经下载成功接口，客户端可以通过{@link Device#confirmIntanceMessage(String[])}接口通知服务器对应的IM消息
	 * 已经下载成功，SDK会将请求结果通过该接口回调客户端。
	 * 
	 * @param reason 平台错误码 参考{@link CloopenReason}
	 * 
	 * @see Device#confirmIntanceMessage(String[])
	 */
	public abstract void onConfirmIntanceMessage(CloopenReason reason);
}
