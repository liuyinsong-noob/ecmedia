package com.hisun.phone.core.voice.util;

import android.os.Build;
import android.text.TextUtils;

public class AdaptationTools {

	/**
	 * mobile Mi108 for xinwei company.
	 * @return
	 */
	public static boolean DeviceIsMi108() {
		
		return "Mi108".equals(android.os.Build.MODEL);
	}
	
	/**
	 *  mobile Mi108 for xinwei company.
	 * @return
	 */
	public static boolean DeviceIsMi106() {
		
		return "Mi106".equals(android.os.Build.MODEL);
	}

	/**
	 * 
	 * @return
	 */
	public static boolean DeviceIsHTC7088() {
		
		return "HTC 7088".equals(android.os.Build.MODEL);
	}
	
	/**
	 * 
	 * @return
	 */
	public static boolean DeviceXIAOMI() {
		return android.os.Build.MODEL.startsWith("MI") || android.os.Build.MODEL.startsWith("mi");
	}
	
	/**
	 * 
	 * @return
	 */
	public static boolean DeviceLenovoK900() {
		return android.os.Build.MODEL.startsWith("Lenovo K900");
	}
	
	public static boolean callNoiseSuppression() {
		return DeviceXIAOMI() || DeviceLenovoK900();
	}
	
	/**
	 * Device for xinwei
	 * @return
	 */
	public static boolean isXinweiDevice() {
		if(!TextUtils.isEmpty(Build.MANUFACTURER) && 
				Build.MANUFACTURER.toLowerCase().indexOf("XINWEI".toLowerCase()) >= 0 ) {
			return true;
		}
		return false;
	}
}
