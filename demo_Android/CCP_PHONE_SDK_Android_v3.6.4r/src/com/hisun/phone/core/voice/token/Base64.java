package com.hisun.phone.core.voice.token;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

/**
 * 字符编码转制的工具类
 * @author wanna
 *
 */
public class Base64
{

	private static final char base64[] = (new String("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")).toCharArray();

	private static char pad1 = '=';

	private static String pad2 = "==";

	// private static final byte F = -1;
	// private static final byte PAD = 64;
	//128位
	private static final byte reverse[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
		                                       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		                                       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		                                       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
		                                       -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 
		                                       54, 55, 56, 57, 58, 59, 60, 61, -1, -1, 
		                                       -1, 64, -1, -1, -1, 0, 1, 2, 3, 4, 
		                                       5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
		                                       15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
		                                       25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
		                                       29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 
		                                       39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 
		                                       49, 50, 51, -1, -1, -1, -1, -1 };

	// private static final String hex = "0123456789abcdef";

	private Base64()
	{
	}
	
	public static String encode(byte input[])
	{
		if (input.length == 0)
		{
			return "";
		}
		int i = (input.length + 2) / 3 << 2;
		StringBuffer output = new StringBuffer(i);
		i = input.length / 3;
		int j = 0;
		for (; i > 0; i--)
		{
			int a = input[j++];
			int b = input[j++];
			int c = input[j++];
			int m = a >>> 2 & 0x3f;
			output.append(base64[m]);
			m = (a & 3) << 4 | b >>> 4 & 0xf;
			output.append(base64[m]);
			m = (b & 0xf) << 2 | c >>> 6 & 3;
			output.append(base64[m]);
			m = c & 0x3f;
			output.append(base64[m]);
		}

		i = input.length % 3;
		switch (i) {
		case 1:
		{
			int a = input[j++];
			int m = a >>> 2 & 0x3f;
			output.append(base64[m]);
			m = (a & 3) << 4;
			output.append(base64[m]);
			output.append(pad2);
			break;
		}

		case 2:
		{
			int a = input[j++];
			int b = input[j++];
			int m = a >>> 2 & 0x3f;
			output.append(base64[m]);
			m = (a & 3) << 4 | b >>> 4 & 0xf;
			output.append(base64[m]);
			m = (b & 0xf) << 2;
			output.append(base64[m]);
			output.append(pad1);
			break;
		}
		}
		return output.toString();
	}
	/**
	 * 解码 将字符串转换成字节数组
	 * @param input
	 * @return   
	 * @throws Exception
	 */
	public static byte[] decode(String input) throws Exception
	{
		if (input.length() == 0)
		{
			return new byte[0];
		}
		byte b[] = new byte[input.length()];
		for (int i = input.length() - 1; i >= 0; i--)
		{
			b[i] = (byte) input.charAt(i);

		}
		return decode(b);
	}

	public static byte[] decode(byte code[]) throws Exception
	{
		int l = code.length;
		boolean end = false;
		int i = 0;
		int j = 0;
		for (; i < l; i++)
		{
			byte m = reverse[code[i]];
			if (m == 64)
			{
				if (end)
				{
					break;
				}
				end = true;
			}
			else
			{
				if (end)
				{
					throw new Exception("Cannot found second char!");
				}
				if (m != -1)
				{
					code[j++] = m;
				}
			}
		}

		l = j >> 2;
		i = l * 3;
		int k = j & 3;
		if (k == 1)
		{
			throw new Exception("Cannot found first char!");
		}
		if (k > 0)
		{
			i = (i + k) - 1;
		}
		byte output[] = new byte[i];
		i = 0;
		j = 0;
		byte b = 0;
		for (; l > 0; l--)
		{
			byte a = code[i++];
			b = code[i++];
			byte c = code[i++];
			byte d = code[i++];
			output[j++] = (byte) (a << 2 | b >>> 4 & 3);
			output[j++] = (byte) ((b & 0xf) << 4 | c >>> 2 & 0xf);
			output[j++] = (byte) ((c & 3) << 6 | d);
		}

		if (k >= 2)
		{
			byte a = code[i++];
			b = code[i++];
			output[j++] = (byte) (a << 2 | b >>> 4 & 3);
		}
		if (k >= 3)
		{
			byte c = code[i++];
			output[j++] = (byte) ((b & 0xf) << 4 | c >>> 2 & 0xf);
		}
		return output;
	}

	public static String toHex(byte b[])
	{
		StringBuffer buf = new StringBuffer(b.length * 2);
		for (int i = 0; i < b.length; i++)
		{
			buf.append("0123456789abcdef".charAt(b[i] >> 4 & 0xf));
			buf.append("0123456789abcdef".charAt(b[i] & 0xf));
		}

		return buf.toString();
	}

	
	
	/**
	 * read file to byte[];
	 */
	public static byte[] readFileToByteArray(File file) throws IOException{
		InputStream in = null;
        try{
		if (file.exists()) {
			if (file.isDirectory()) {
				throw new IOException("File '" + file
						+ "' exists but is a directory");
			}
			if (file.canRead() == false) {
				throw new IOException("File '" + file + "' cannot be read");
			}
		} else {
			throw new FileNotFoundException("File '" + file
					+ "' does not exist");
		}
		in = new FileInputStream(file);
		ByteArrayOutputStream output = new ByteArrayOutputStream();
		byte[] buffer = new byte[1024 * 4];
		int n = 0;
		  while (-1 != (n = in.read(buffer))) {
           output.write(buffer, 0, n);
        }
		return output.toByteArray();
        }finally{
        	if(in != null){
        		in.close();
        		in = null;
        	}
        }

	}
}