/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */package com.voice.demo.tools;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * simple string validate methods
 *
 * @author chao
 * @version 1.0.0
 */
public class CheckUtil {

	public static final String[] PHONE_PREFIX = new String[] { "130", "131",
			"132", "133", "134", "135", "136", "137", "138", "139", "150",
			"151", "152", "153", "154", "155", "156", "157", "158", "159",
			"180", "181", "182", "183", "184", "185", "186", "187", "188",
			"189" };

	public static boolean checkLocation(String mdn) {
		return checkMDN(mdn, false);
	}

	public static boolean checkMDN(String mdn) {
		return checkMDN(mdn, true);
	}

	/**
	 * 检查手机号码合法性
	 *
	 * @param mdn
	 * @return
	 */
	public static boolean checkMDN(String mdn, boolean checkLen) {
		if (mdn == null || mdn.equals("")) {
			return false;
		}
		//modify by zhangyp 去掉号码前边的+86
		if (mdn.startsWith("+86")) {
			mdn = mdn.substring(3);
		}
		if (checkLen && mdn.length() != 11) {
			return false;
		}
		boolean flag = false;
		String p = mdn.length() > 3 ? mdn.substring(0, 3) : mdn;
		for (int i = 0; i < PHONE_PREFIX.length; i++) {
			if (p.equals(PHONE_PREFIX[i])) {
				flag = true;
				break;
			}
		}
		if (!flag) {
			return false;
		}
		return true;
	}

	public static final char[] INVALID_CH_CN = {
		  'A','B','C','D','E','F','G','H','I','J','K','L',
		  'M','N','O','P','Q','R','S','T','U','V','W','X',
		  'Y','Z',
		  'a','b','c','d','e','f','g','h','i','j','k','l',
		  'm','n','o','p','q','r','s','t','u','v','w','x',
		  'y','z',
		  '0','1','2','3','4','5','6','7','8','9',
		  '.',',',';',':','!','@','/','(',')','[',']','{',
		  '}','|','#','$','%','^','&','<','>','?','\'','+',
		  '-','*','\\','\"'};

	public static boolean checkCN(String str) {
		if (str == null || str.length() == 0) {
			return false;
		}
		char[] cArray = str.toCharArray();
		for (int i = 0; i < cArray.length; i++) {
			for (int j = 0; j < INVALID_CH_CN.length; j++) {
				if (cArray[i] == INVALID_CH_CN[j]) {
					return false;
				}
			}
		}
		return true;
	}

	/**
	 * 是否为新版本, true  为有新版本， 否则；
	 * 
	 * @param mdn
	 * @return
	 */
	public static boolean versionCompare(String oldversion, String newversion) {
		if (oldversion == null || newversion == null) {
			return false;
		}
		String[] oldstr = oldversion.split("\\.");
		String[] newstr = newversion.split("\\.");

		int[] oldint = new int[oldstr.length];
		int[] newint = new int[newstr.length];

		try {
			for (int i = 0; i < oldstr.length; i++) {
				oldint[i] = Integer.valueOf(oldstr[i]);
			}
			for (int i = 0; i < newstr.length; i++) {
				newint[i] = Integer.valueOf(newstr[i]);
			}
		} catch (Exception e) {
		}

		// 可以简化的逻辑
		int count = oldint.length > newint.length ? newint.length
				: oldint.length;
		for (int temp = 0; temp < count; temp++) {
			if (newint[temp] == oldint[temp]) {
				continue;
			} else if (newint[temp] > oldint[temp]) {
				return true;
			} else {
				return false;
			}
		}
        if (newint.length > oldint.length){
        	return true;
        }
		return false;
	}

	/**
	 * 检测邮箱合法性
	 * @param email
	 * @return
	 */
	public static boolean checkEmailValid(String email) {
		if ((email == null) || (email.trim().length() == 0)) {
			return false;
		}
		String regEx = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}$";
		Pattern p = Pattern.compile(regEx);
		Matcher m = p.matcher(email.trim().toLowerCase());

		return m.find();
	}
}
