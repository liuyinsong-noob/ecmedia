/**
 * 
 */
package com.hisun.phone.core.voice;

import com.hisun.phone.core.voice.util.ObjectStringIdentifier;
import com.hisun.phone.core.voice.util.SdkErrorCode;

/**
 * 
 * Don't do anything pass time things on those methods.
 * 
 * @author chao
 * 
 */
public interface DeviceListener {

	/**
	 * callback for presence state, invoked when capability toke changed.
	 */
	public abstract void onReceiveEvents(CCPEvents events);

	/**
	 * Callback for connect to CCP server success.
	 */
	public abstract void onConnected();

	/**
	 * Callback for connect to CCP server failed.
	 * 
	 * @param reason
	 */
	public abstract void onDisconnect(Reason reason);

	public static enum NetworkState {
		CONNECTED, DISCONNECTED, UNKNOWN
	}

	public static enum APN {
		UNKNOWN, WIFI, CMWAP, UNIWAP, CTWAP, CMNET, UNINET, CTNET, INTERNET, GPRS, WONET, WOWAP
	}

	/**
	 * 
	* <p>Title: DeviceListener.java</p>
	* <p>Description: </p>
	* <p>Copyright: Copyright (c) 2012</p>
	* <p>Company: http://www.yuntongxun.com</p>
	* @author Jorstin Chan
	* @date 2013-10-23
	* @version 3.5
	* 
	* @see Reason#INVALIDPROXY version 3.5 for Verification of soft switching address is legitimate
	* 
	 */
	public static enum Reason implements ObjectStringIdentifier{
		UNKNOWN(0 ,"系统错误，请稍后再试"), 
		NOTRESPONSE(1 ,"云通讯服务器无响应"), 
		AUTHFAILED(2 , "鉴权失败"), 
		DECLINED(3 , "对方拒绝您的呼叫请求"), 
		NOTFOUND(4 , "对方不在线或为空号"), 
		CALLMISSED(5 , "呼叫无应答"), 
		BUSY(6 , "对方正忙"), 
		NOTNETWORK(7 ,"网络连接异常"), 
		KICKEDOFF(9 , "帐号在其他地方登录"), 
		OTHERVERSIONNOTSUPPORT(10 ,"对方版本不支持音频或视频"), 
		INVALIDPROXY(11 , "参数错误,无效的代理"),
		TIME_OUT(408 , "呼叫超时"),
		MEDIACONSULTFAILED(488 ,"媒体协商失败"), 
		
		AUTHADDRESSFAILED(700 ,"第三方鉴权地址连接失败"),
		MAINACCOUNTINVALID(702 , "第三方应用ID未找到"), 
		CALLERSAMECALLED(704 ,"第三方应用未上线限制呼叫已配置测试号码"), 
		SUBACCOUNTPAYMENT(705 ,"第三方鉴权失败，子账号余额"), 
		MAINACCOUNTPAYMENT(710 , "第三方主账号余额不足"),
		CONFERENCE_NOT_EXIST(707 ,"呼入会议号已解散不存在"),
		PASSWORD_ERROR(708 , "呼入会议号密码验证失败"),
		LOCAL_CALL_BUSY(SdkErrorCode.SDK_CALL_BUSY , "本地线路忙"),
		VERSIONNOTSUPPORT(170007 ,"该版本不支持音频或视频");

		
		private int status;
		private String mValue;
		
		private Reason(int status , String value){
			this.status = status;
			this.mValue = value;
		}
		
		public void setStatus(int status) {
			this.status = status;
		}

		public int getStatus() {
			return this.status;
		}

		public void setValue(String value) {
			this.mValue = value;
		}
		
		@Override
		public String getValue() {
			return this.mValue;
		}

	}

	public static enum ReportState {
		SUCCESS, FAILED
	}

	public static enum CCPEvents {
		SYSCallComing, /* NSChanged, BatteryLower */
	}

	// ----------------------------------------------
	// p2p set callback
	public abstract void onFirewallPolicyEnabled();

}
