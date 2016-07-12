package com.hisun.phone.core.voice.util;

import android.content.Context;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

public class UserAgentUtils {

	/**
	 * User-Agent
	 * 
	 * @return user-agent
	 */
	public static String getUser_Agent(Context context) {
		String ua = "Android;" 
			+ getOSVersion() + ";" 
			+ com.hisun.phone.core.voice.Build.SDK_VERSION + ";"
			+ com.hisun.phone.core.voice.Build.LIBVERSION.FULL_VERSION + ";"
			+ getVendor() + "-" + getDevice() + ";";
		
		ua = ua + getDevicNO(context)  + ";" + System.currentTimeMillis() + ";";
		
		Log4Util.d("User_Agent : " + ua);
		return ua;
	}
	
	public static String getDevicNO(Context context) {
		if(!TextUtils.isEmpty(getDeviceId(context))) {
			return getDeviceId(context);
		}
		
		if(!TextUtils.isEmpty(getMacAddress(context))) {
			return getMacAddress(context);
		}
		return "";
	}
	
	public static String getDeviceId(Context context) {
		TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
		if(telephonyManager != null ) {
			return telephonyManager.getDeviceId();
		}
		
		return null;

	}
	
	public static String getMacAddress(Context context) {
		// start get mac address
		WifiManager wifiMan = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
		if (wifiMan != null) {
			WifiInfo wifiInf = wifiMan.getConnectionInfo();
			if (wifiInf != null && wifiInf.getMacAddress() != null) {
				// 48位，如FA:34:7C:6D:E4:D7
				return wifiInf.getMacAddress();
			}
		}
		return null;
	}

	/**
	 * device model name, e.g: GT-I9100
	 * 
	 * @return the user_Agent
	 */
	public static String getDevice() {
		return Build.MODEL;
	}

	/**
	 * device factory name, e.g: Samsung
	 * 
	 * @return the vENDOR
	 */
	public static String getVendor() {
		return Build.BRAND;
	}

	/**
	 * @return the SDK version
	 */
	public static int getAndroidSDKVersion() {
		return Build.VERSION.SDK_INT;
	}

	/**
	 * @return the OS version
	 */
	public static String getOSVersion() {
		return Build.VERSION.RELEASE;
	}
}
