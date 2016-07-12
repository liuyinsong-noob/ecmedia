/**
 * 
 */
package com.hisun.phone.core.voice.token;

import java.util.Map;

/**
 * @author chao
 *
 */
public interface CapabilityToken {

	public static final int MAX_AUTHTOKEN_LEN   = 10240;
	public static final int KEY_COUNT  = 5;
	public static final int JSON_KEY_LEN = 1024;
	
	public static final String CALLING = "calling";
	public static final String CALLED = "called";
	public static final String MSGFLAG = "msgflag";
	public static final String CREATECONF = "createconf";
	public static final String JOINCONF = "joinconf";
	
	public static final String [] KEY_TOKEN = {"calling", "message", "createconf", "joinconf", "timestamp", "expires"};
	
	/************************************************************************
	* 函  数  名：DecodeAuthToken
	* 功     能：解析获取能力级信息
	* 入     参：加密的能力级字符串
	* 入     参：解密的密码，暂时没有使用
	* 返  回  值: 哈希表 能力级信息串，格式为010101  0代表没有该能力，1代表有，2 代表异常
	* 作     者：
	************************************************************************/
	public Map<String, String> decodeAuthToken(final String token, final String key)throws Exception;
	
	/************************************************************************
	* 函  数  名：EncodeAuthToken
	* 功     能：获取用户的能力级信息
	* 入     参：能力级信息串
	* 返  回  值：编码后的能力级信息
	* 作     者：
	************************************************************************/

	public String encodeAuthToken(final String token) throws Exception;
}
