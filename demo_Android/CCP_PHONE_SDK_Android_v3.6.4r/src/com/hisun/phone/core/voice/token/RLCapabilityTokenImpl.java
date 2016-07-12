/**
 * 
 */
package com.hisun.phone.core.voice.token;

import java.util.Map;


/**
 * @author chao
 * 
 */
public final class RLCapabilityTokenImpl implements CapabilityToken {

	/**
	 * 
	 */
	public RLCapabilityTokenImpl() {
		super();
	}

	public Map<String, String> decodeAuthToken(final String token, String key)throws Exception {
		if (token == null) {
			throw new NullPointerException("");
		}

		int length = token.length();

		if (length > MAX_AUTHTOKEN_LEN) {
			throw new IllegalArgumentException("token length over the max.");
		}

		if (key == null) {
			System.out.println("currently don't handle this.");
		}
		
		return /*array*/null;
	}

	/**
	 * token 01011
	 */
	public String encodeAuthToken(String token) throws Exception {
		if (token == null) {
			throw new NullPointerException("token is null.");
		}
		
		int length = token.length();
		if(length != KEY_TOKEN.length) {
			throw new IllegalArgumentException("params length is different from KEY_TOKEN array length.");
		}
		
		StringBuffer sb = new StringBuffer("{");
		for (int i = 0; i < length; i++) {
			sb.append("\"" + KEY_TOKEN[i] + "\"").append(":");
			sb.append("\"" + token.charAt(i) + "\"");
			if (i < length - 1) {
				sb.append(",");
			}
		}
		sb.append("}");
		System.out.println("" + sb.toString());
		return Base64.encode(sb.toString().getBytes());
	}
	
	public static void main(String [] args) {
		CapabilityToken capabilityToken = new RLCapabilityTokenImpl ();
		try {
			String encode = capabilityToken.encodeAuthToken("10101");
			System.out.println(encode);
			
			Map<String, String> decode = capabilityToken.decodeAuthToken(encode, null);
			System.out.println(decode);
		} catch (Exception e) {
			e.printStackTrace();
		}
		
	}
}
