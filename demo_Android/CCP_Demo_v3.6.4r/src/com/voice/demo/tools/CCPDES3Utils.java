package com.voice.demo.tools;

import java.security.Key;

import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESedeKeySpec;

import com.hisun.phone.core.voice.util.Cryptos;

/**
 * 
 * @ClassName: CCPDES3Utils.java
 * @Description: 3DES encryption tools
 * @author Jorstin Chan 
 * @date 2013-12-16
 * @version 3.6
 */
public class CCPDES3Utils {

	// 密钥
	public final static String SECRET_KEY = "a04daaca96294836bef207594a0a4df8";

	// vectors
	private final static String iv = "01234567";

	// The encryption / decryption algorithm / model / fill mode
	public static final String CIPHER_ALGORITHM = "DES/ECB/PKCS5Padding";

	// Encryption and decryption unified encoding to use
	private final static String encoding = "utf-8";
	
	/**
	 * 
	 * @param plainText Encrypted text
	 * @return
	 * @throws Exception
	 */
	public static String encode(String plainText) throws Exception {
		
		return Cryptos.toBase64QES(SECRET_KEY, plainText);
	}
	
	public static String decode(String plainText) throws Exception {
		
		return Cryptos.toDecodeQES(SECRET_KEY, plainText);
	}
	
	/**
	 * 
	 * @param plainText Encrypted text
	 * @return
	 * @throws Exception
	 */
	public static String encode(String plainText, String secretKey)
			throws Exception {
		Key deskey = null;
		DESedeKeySpec spec = new DESedeKeySpec(secretKey.getBytes());
		SecretKeyFactory keyfactory = SecretKeyFactory.getInstance("DESede");
		deskey = keyfactory.generateSecret(spec);

		Cipher cipher = Cipher.getInstance("DESede");
		//IvParameterSpec ips = new IvParameterSpec(iv.getBytes());
		cipher.init(Cipher.ENCRYPT_MODE, deskey/*, ips*/);
		byte[] encryptData = cipher.doFinal(plainText.getBytes(encoding));
		return com.hisun.phone.core.voice.token.Base64.encode(encryptData);
	}

	/**
	 * 3DES decryption
	 * 
	 * @param encryptText Encrypted text
	 * @return
	 * @throws Exception
	 */
	public static String decode(String encryptText, String secretKey)
			throws Exception {
		Key deskey = null;
		DESedeKeySpec spec = new DESedeKeySpec(secretKey.getBytes());
		SecretKeyFactory keyfactory = SecretKeyFactory.getInstance("DESede");
		deskey = keyfactory.generateSecret(spec);
		Cipher cipher = Cipher.getInstance(CIPHER_ALGORITHM);
		//IvParameterSpec ips = new IvParameterSpec(iv.getBytes());
		cipher.init(Cipher.DECRYPT_MODE, deskey/*, ips*/);

		byte[] decryptData = cipher.doFinal(com.hisun.phone.core.voice.token.Base64.decode(encryptText));

		return new String(decryptData, encoding);
	}
	
}
