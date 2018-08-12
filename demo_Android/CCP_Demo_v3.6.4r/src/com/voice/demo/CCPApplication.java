/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.media.AudioManager;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.widget.Toast;

import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.contacts.model.ContactBean;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.CrashHandler;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.ui.model.DemoAccounts;

/**
 * Deom Application
 * 
 * @see #DemoApplication()
 * 
 */
public class CCPApplication extends Application {
	
	private List<ContactBean> contactBeanList;
	public List<ContactBean> getContactBeanList() {
		return contactBeanList;
	}

	public void setContactBeanList(List<ContactBean> contactBeanList) {
		this.contactBeanList = contactBeanList;
	}

	private static CCPApplication instance;
	public static ArrayList<String> interphoneIds = null;
	public static ArrayList<String> chatRoomIds;

	public final static String VALUE_DIAL_MODE_FREE = "voip_talk";
	public final static String VALUE_DIAL_MODE_BACK = "back_talk";
	public final static String VALUE_DIAL_MODE_DIRECT = "direct_talk";
	public final static String VALUE_DIAL_MODE = "mode";
	public final static String VALUE_DIAL_SOURCE_PHONE = "srcPhone";
	public final static String VALUE_DIAL_VOIP_INPUT = "VoIPInput";
	public final static String VALUE_DIAL_MODE_VEDIO = "vedio_talk";

	private File vStore;

	private DemoAccounts accounts;
	
	
	boolean isDeveloperMode = false;
	
	 
	public boolean isDeveloperMode() {
		return isDeveloperMode;
	}

	public void setDeveloperMode(boolean isDeveloperMode) {
		this.isDeveloperMode = isDeveloperMode;
	}

	boolean isChecknet = false;

	public boolean isChecknet() {
		
		return isChecknet;
	}

	public void setChecknet(boolean isChecknet) {

		this.isChecknet = isChecknet;
	}

	@Override
	public void onCreate() {
		super.onCreate();
		
		instance = this;
		//initSQLiteManager();
		initFileStore();
		initCrashHandler();
		
		//Sets the default preferences if no value is set yet
        CcpPreferences.loadDefaults();
        
        boolean firstUse = CcpPreferences.getSharedPreferences().getBoolean(
        		CCPPreferenceSettings.SETTINGS_FIRST_USE.getId(),
                ((Boolean)CCPPreferenceSettings.SETTINGS_FIRST_USE.getDefaultValue()).booleanValue());
        
      //Display the welcome message?
        if (firstUse) {
        	if(getVoiceStore() != null) {
        		CCPUtil.delAllFile(getVoiceStore().getAbsolutePath());
        	}

            // Don't display again this dialog
            try {
            	CcpPreferences.savePreference(
            			CCPPreferenceSettings.SETTINGS_FIRST_USE, Boolean.FALSE, true);
            } catch (Exception e) {/**NON BLOCK**/}
        }
        
        
		if (interphoneIds == null) {
			interphoneIds = new ArrayList<String>();
		}
		if (chatRoomIds == null) {
			chatRoomIds = new ArrayList<String>();
		}
		
//		Intent startService = new Intent(this, T9Service.class);
//		startService(startService);
	}

	private void initCrashHandler() {
		CrashHandler crashHandler = CrashHandler.getInstance();
		crashHandler.init(getApplicationContext());
	}

	public void initSQLiteManager() {
		CCPSqliteManager.getInstance();
		Log4Util.d(CCPHelper.DEMO_TAG, "CCPApplication.initSQLiteManager");
	}

	private void initFileStore() {
		if (!CCPUtil.isExistExternalStore()) {
			Toast.makeText(getApplicationContext(), R.string.media_ejected,
					Toast.LENGTH_LONG).show();
			return;
		}
		File directory = new File(Environment.getExternalStorageDirectory(),
				CCPUtil.DEMO_ROOT_STORE);
		if (!directory.exists() && !directory.mkdirs()) {
			Toast.makeText(getApplicationContext(),
					"Path to file could not be created", Toast.LENGTH_SHORT)
					.show();
			return;
		}
		vStore = directory;
	}

	public File getVoiceStore() {
		if(vStore == null || vStore.exists()) {
			initFileStore();
		}
		return vStore;
	}
	

	public void showToast(String text) {
		Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT)
				.show();
	}

	public void showToast(int resId) {
		Toast.makeText(getApplicationContext(), resId, Toast.LENGTH_SHORT)
				.show();
	}

	public static CCPApplication getInstance() {
		return instance;
	}


	/**
	 * User-Agent
	 * 
	 * @return user-agent
	 */
	public String getUser_Agent() {
		String ua = "Android;" 
			+ getOSVersion() + ";" 
			+ com.hisun.phone.core.voice.Build.SDK_VERSION + ";"
			+ com.hisun.phone.core.voice.Build.LIBVERSION.FULL_VERSION + ";"
			+ getVendor() + "-" + getDevice() + ";";
		
		ua = ua + getDevicNO()  + ";" + System.currentTimeMillis() + ";";
		
		Log4Util.d(CCPHelper.DEMO_TAG, "User_Agent : " + ua);
		return ua;
	}
	
	public String getDevicNO() {
		if(!TextUtils.isEmpty(getDeviceId())) {
			return getDeviceId();
		}
		
		if(!TextUtils.isEmpty(getMacAddress())) {
			return getMacAddress();
		}
		return " ";
	}
	
	public String getDeviceId() {
		TelephonyManager telephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
		if(telephonyManager != null ) {
			return telephonyManager.getDeviceId();
		}
		
		return null;

	}
	
	public String getMacAddress() {
		// start get mac address
		WifiManager wifiMan = (WifiManager) getSystemService(Context.WIFI_SERVICE);
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
	public String getDevice() {
		return Build.MODEL;
	}

	/**
	 * device factory name, e.g: Samsung
	 * 
	 * @return the vENDOR
	 */
	public String getVendor() {
		return Build.BRAND;
	}

	/**
	 * @return the SDK version
	 */
	public int getSDKVersion() {
		return Build.VERSION.SDK_INT;
	}

	/**
	 * @return the OS version
	 */
	public String getOSVersion() {
		return Build.VERSION.RELEASE;
	}

	/**
	 * Retrieves application's version number from the manifest
	 * 
	 * @return versionName
	 */
	public String getVersion() {
		String version = "0.0.0";
		try {
			PackageInfo packageInfo = getPackageManager().getPackageInfo(
					getPackageName(), 0);
			version = packageInfo.versionName;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}

		return version;
	}

	/**
	 * Retrieves application's version code from the manifest
	 * 
	 * @return versionCode
	 */
	public int getVersionCode() {
		int code = 1;
		try {
			PackageInfo packageInfo = getPackageManager().getPackageInfo(
					getPackageName(), 0);
			code = packageInfo.versionCode;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}

		return code;
	}

	/**
	 * 
	 * @param mode
	 */
	public void setAudioMode(int mode) {
		AudioManager audioManager = (AudioManager) getApplicationContext()
				.getSystemService(Context.AUDIO_SERVICE);
		if (audioManager != null) {
			audioManager.setMode(mode);
		}
	}

	/**
	 * 
	 * @param phoneNum
	 */
	public void startCalling(String phoneNum) {
		try {
			Intent intent = new Intent(Intent.ACTION_CALL, Uri.parse("tel://"
					+ phoneNum));
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			startActivity(intent);
		} catch (Exception e) {
			e.printStackTrace();
			showToast(R.string.toast_call_phone_error);
		}
	}

	public void quitApp() {
		System.exit(0);
	}

	public static HashMap<String, InstanceMsg> rMediaMsgList = new HashMap<String, InstanceMsg>();

	public InstanceMsg getMediaData(String key) {
		if (key != null) {
			return rMediaMsgList.get(key);
		} else {
			return null;
		}
	}

	/**
	 * 
	 * @param key
	 * @param list
	 */
	public void putMediaData(String key, InstanceMsg obj) {
		if (key != null && obj != null) {
			rMediaMsgList.put(key, obj);
		}
	}

	public void removeMediaData(String key) {
		if (key != null) {
			rMediaMsgList.remove(key);
		}
	}

	public HashMap<String, InstanceMsg> getMediaMsgList() {
		return rMediaMsgList;
	}
	
	
	private HashMap<String, Object> dataMap = new HashMap<String, Object>();
	/**
	 * @param key
	 * @param list
	 */
	public void putData(String key, Object obj) {
		if (key != null && obj != null) {
			dataMap.put(key, obj);
		}
	}

	public void removeData(String key) {
		if (key != null) {
			dataMap.remove(key);
		}
	}
	public Object getData(String key) {
		if (key != null) {
			return dataMap.get(key);
		} else {
			return null;
		}
	}
	
	/**
	 * To obtain the system preferences to save the file to edit the object
	 * @return
	 */
	public Editor getSharedPreferencesEditor() {
		SharedPreferences cCPreferences = getSharedPreferences(CcpPreferences.CCP_DEMO_PREFERENCE, MODE_PRIVATE);
		Editor edit = cCPreferences.edit();
		
		return edit;
	}
	/**
	 * To obtain the system preferences to save the file to edit the object
	 * @return
	 */
	public SharedPreferences getSharedPreferences() {
		return getSharedPreferences(CcpPreferences.CCP_DEMO_PREFERENCE, MODE_PRIVATE);
	}
	
	
	/**
	 * 获取动态时间
	 * 
	 * @param key
	 * @return
	 */
    public String getSettingParams(String key) {
		SharedPreferences settings = getSharedPreferences();
		return settings.getString(key, "");
	}
    
    /**
     * 保存动态时间
     * @param key
     * @param value
     */
	public void saveSettingParams(String key, String value) {
		SharedPreferences settings = getSharedPreferences();
		settings.edit().putString(key, value).commit();
	}
	
	 /**
     * 清除动态时间
     * @param key
     * @param value
     */
	public void clearSettingParams() {
		SharedPreferences settings = getSharedPreferences();
		settings.edit().clear().commit();
	}
	
	/**
	 * 删除配置
	 * @param key
	 */
	public void removeSettingParam(String key) {
		SharedPreferences settings = getSharedPreferences("Dynamic_Time_Preferences", 0);
		settings.edit().remove(key).commit();
	}
	
	
	
	public void putDemoAccounts(DemoAccounts demoAccounts) {
		accounts = demoAccounts;
	}
	
	public DemoAccounts getDemoAccounts() {
		return accounts;
	}
	
	@Override
	public void onLowMemory() {
		super.onLowMemory();
		
	}
	
}
