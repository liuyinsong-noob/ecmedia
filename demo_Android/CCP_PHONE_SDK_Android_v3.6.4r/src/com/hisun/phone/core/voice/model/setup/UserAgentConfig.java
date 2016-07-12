/**
 * 
 */
package com.hisun.phone.core.voice.model.setup;

import java.util.Map;

import android.text.TextUtils;

public class UserAgentConfig {

	/**
	 * this is main account of ccp allocated
	 */
	public static final String KEY_SID = "ccp_key_sid";
	/**
	 * this is main account of ccp allocated
	 */
	public static final String KEY_PWD = "ccp_key_pwd";
	/**
	 * this is sub account of ccp allocated
	 */
	public static final String KEY_SUBID = "ccp_key_subid";
	/**
	 * this is sub token of ccp allocated
	 */
	public static final String KEY_SUBPWD = "ccp_key_subpwd";
	/**
	 * this is ccp address
	 */
	public static final String KEY_IP = "ccp_key_ip";
	/**
	 * this is ccp port
	 */
	public static final String KEY_PORT = "ccp_key_port";
	/**
	 * this is device user-agent
	 */
	public static final String KEY_UA = "ccp_key_useragent";
	
	/**
	 * REST get a private cloud soft exchange interface (Xin Wei)
	 */
	public static final String KEY_PRIVATECLOUD = "ccp_key_privateCloud";
	
	public static final String KEY_VALIDATE = "ccp_key_validate";

	// 42.121.254.126
	public final String getRequestUrl(String protocol) {
		if(!checkHttpScheme()) {
			if ("HTTP".equalsIgnoreCase(protocol)) {
				return "http://" + ua_server + ":8881"/* + ua_port*/;
			}
			
			return "https://" + ua_server + ":" + ua_port;
		}
		return ua_server + ":" + ua_port;
	}
	
	/**
	 * main account
	 */
	private String sid;
	/**
	 * main token
	 */
	private String password;

	/**
	 * rest server address
	 */
	private String ua_server;

	/**
	 * rest server port
	 */
	private int ua_port;

	/**
	 * sub account
	 */
	private String subaccountid;

	/**
	 * sub token
	 */
	private String subpassword;
	
	/**
	 * REST get a private cloud soft exchange interface (Xin Wei)
	 * @version 3.6
	 */
	private String privateCloud;
	
	private String validate;

	/**
	 * device user-agent
	 */
	private String ua;
	
	// 音频解码器
	private String audio_decoder;
	// Sip铃声地址
	private String ringtone;
	// 优先选项（音质优先，流量优先）
	private int prior;


	public UserAgentConfig(Map<String, String> params) {
		if (params == null || params.size() == 0) {
			throw new RuntimeException("sdk need params is null.");
		}
		if (!params.containsKey(KEY_IP) || !params.containsKey(KEY_PORT)
				|| !params.containsKey(KEY_PWD) || !params.containsKey(KEY_SID)
				|| !params.containsKey(KEY_UA)) {
			throw new RuntimeException("sdk need params errors.");
		}

		try {
			// version 3.6
			if(params.containsKey(KEY_VALIDATE) && params.containsKey(KEY_PRIVATECLOUD)) {
				this.privateCloud = params.get(KEY_PRIVATECLOUD);
				this.validate = params.get(KEY_VALIDATE);
			}
			
			this.sid = params.get(KEY_SID);
			this.password = params.get(KEY_PWD);
			this.ua_server = params.get(KEY_IP);
			this.ua_port = Integer.parseInt(params.get(KEY_PORT));
			this.subaccountid = params.get(KEY_SUBID);
			this.subpassword = params.get(KEY_SUBPWD);
			this.ua = params.get(KEY_UA);
			
		} catch (Exception e) {
			e.printStackTrace();
			throw new RuntimeException(
					"create UserAgentConfig failed. params is incorrect."
							+ e.toString());
		} finally {
			if (params != null) {
				params.clear();
				params = null;
			}
		}
	}
	
	/**
	 * @return
	 */
	private boolean checkHttpScheme() {
		if(!TextUtils.isEmpty(ua_server) 
				&& !ua_server.toLowerCase().startsWith("https://")
				&& !ua_server.toLowerCase().startsWith("http://")) {
			return false;
		}
		return true;
	}

	/**
	 * @return the sid
	 */
	public String getSid() {
		return sid;
	}

	/**
	 * @param sid
	 *            the sid to set
	 */
	public void setSid(String sid) {
		this.sid = sid;
	}

	/**
	 * @return the ua_server
	 */
	public String getUa_server() {
		return ua_server;
	}

	/**
	 * @param ua_server
	 *            the ua_server to set
	 */
	public void setUa_server(String ua_server) {
		this.ua_server = ua_server;
	}

	/**
	 * @return the password
	 */
	public String getPassword() {
		return password;
	}

	/**
	 * @param password
	 *            the password to set
	 */
	public void setPassword(String password) {
		this.password = password;
	}

	/**
	 * @return the ua_port
	 */
	public int getUa_port() {
		return ua_port;
	}

	/**
	 * @param ua_port
	 *            the ua_port to set
	 */
	public void setUa_port(int ua_port) {
		this.ua_port = ua_port;
	}

	public String getAudio_decoder() {
		return audio_decoder;
	}

	public void setAudio_decoder(String audio_decoder) {
		this.audio_decoder = audio_decoder;
	}

	public String getRingtone() {
		return ringtone;
	}

	public void setRingtone(String ringtone) {
		this.ringtone = ringtone;
	}

	public int getPrior() {
		return prior;
	}

	public void setPrior(int prior) {
		this.prior = prior;
	}

	/**
	 * @return the ua
	 */
	public String getUa() {
		return ua;
	}

	/**
	 * @param ua
	 *            the ua to set
	 */
	public void setUa(String ua) {
		this.ua = ua;
	}

	/**
	 * @return the subaccountid
	 */
	public String getSubaccountid() {
		return subaccountid;
	}

	/**
	 * @param subaccountid
	 *            the subaccountid to set
	 */
	public void setSubaccountid(String subaccountid) {
		this.subaccountid = subaccountid;
	}

	/**
	 * @return the subpassword
	 */
	public String getSubpassword() {
		return subpassword;
	}

	/**
	 * @param subpassword
	 *            the subpassword to set
	 */
	public void setSubpassword(String subpassword) {
		this.subpassword = subpassword;
	}
	
	
	/**
	 * @return
	 */
	public String getPrivateCloud() {
		return privateCloud;
	}

	/**
	 * @param privateCloud
	 */
	public void setPrivateCloud(String privateCloud) {
		this.privateCloud = privateCloud;
	}
	

	public String getValidate() {
		return validate;
	}

	public void setValidate(String validate) {
		this.validate = validate;
	}

	public void released() {
		sid = null;
		password = null;
		ua_server = null;
		audio_decoder = null;
		ringtone = null;
		ua = null;
		subaccountid = null;
		subpassword = null;
		p2pServerPort = null;
		privateCloud = null;
		validate = null;
	}
	
	// P2P server and port
	private String p2pServerPort = "42.121.15.99:3478";

	public String getP2pServerPort() {
		return p2pServerPort;
	}

	public void setP2pServerPort(String p2pServerPort) {
		this.p2pServerPort = p2pServerPort;
	}
	
}
