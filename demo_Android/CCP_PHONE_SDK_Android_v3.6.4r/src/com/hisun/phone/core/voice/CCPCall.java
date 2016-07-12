/**
 * 
 */
package com.hisun.phone.core.voice;

import java.util.List;
import java.util.Map;


import android.content.Context;

/**
 * SDK start Android Service处理通话,当服务启动后,需要创建Device
 * 
 */
public abstract class CCPCall {

	public static void init(Context inContext, InitListener inListener) {
		if (inContext == null) {
			throw new IllegalArgumentException("Context cannot be null.");
		}

		if (inListener == null) {
			throw new IllegalArgumentException("Listener cannot be null.");
		}

		CCPCallImpl.getInstance().init(inContext, inListener);
	}

	public static Device createDevice(DeviceListener deviceListener, Map<String, String> params) {
		return CCPCallImpl.getInstance().createDevice(deviceListener, params);
	}

	public static boolean isInitialized() {
		return CCPCallImpl.getInstance().isInitialized();
	}

	public static void shutdown() {
		CCPCallImpl.getInstance().shutdown();
	}

	public static List<Device> listDevices() {
		return CCPCallImpl.getInstance().listDevices();
	}

	public static abstract interface InitListener {
		public abstract void onInitialized();
		public abstract void onError(Exception paramException);
	}
	
	/*public static void destroyDevice(){
		
	}*/
}
