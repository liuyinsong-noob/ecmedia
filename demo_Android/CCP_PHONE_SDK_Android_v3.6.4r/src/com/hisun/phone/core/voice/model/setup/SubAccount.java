/**
 * 
 */
package com.hisun.phone.core.voice.model.setup;

import com.hisun.phone.core.voice.model.Response;

/**
 * @author chao
 * 
 */
public class SubAccount extends Response {
	
	/**
	 * 
	 */
	private static final long serialVersionUID = -3504711202554697795L;

	/**
	 * 子账户ID
	 */
	public String accountSid;
	
	/**
	 * 应用ID
	 */
	public String appId;
	
	/**
	 * 子账户授权令牌
	 */
	public String authToken;
	
	/**
	 * 创建时间
	 */
	public String dateCreated;
	
	/**
	 * 子账户别名
	 */
	public String friendlyName;
	
	/**
	 * 主账户ID
	 */
	public String parentAccountSid;
	
	/**
	 * 主账户密码
	 */
	public String parentAccountPwd;
	
	/**
	 * SIP账号
	 */
	public String sipCode;
	
	/**
	 * SIP密码
	 */
	public String sipPwd;
	
	/**
	 * 子账户状态 0：未激活、1：激活、2：暂停、3：关闭
	 */
	public String status;
	
	/**
	 * 子账户类型 0:试用、1:已注册
	 */
	public String type;
	
	/**
	 * 请求URI
	 */
	public String uri;

	/**
	 * 会议、短信、呼叫URI
	 */
	public static class SubresourceUris {
		
		/**
		 * 呼叫URI
		 */
		public static String calls;
		
		/**
		 * 会议URI
		 */
		public static String conferences;
		
		/**
		 * 短信URI
		 */
		public static String smsMessages;
	}
}
