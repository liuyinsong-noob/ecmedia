package com.hisun.phone.core.voice.util;

import org.apache.http.HttpHost;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.DeviceListener.APN;
import com.hisun.phone.core.voice.util.Log4Util;

import android.content.Context;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

/**
 * 
* <p>Title: CheckApnTypeUtils.java</p>
* <p>Description: We use the Android device connecting to the network, If it is a WAP access point 
* 	need to set the proxy ,The telecommunications and Unicom mobile agent is not the same,
* 	Mobile and China Unicom's WAP agent are 10.0.0.172:80, WAP agent telecommunication is 10.0.0.200:80
* 
* 	Android development so it is necessary to determine the access point.
* 
*   The type of access points:
*   	1��Net network: operators (Mobile Unicom Telecom) net network, WiFi, USB network sharing
*   	2��Wap network: WAP (Mobile Unicom agent the same: 10.0.0.172:80), WAP (Telecommunications Agency: 10.0.0.200:80)
*   So it can be abstracted three network types: Unicom mobile telecommunications WAP, WAP, the other is net type.</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-11-12
* @version 3.5
 */
public class CheckApnTypeUtils {
	
	private static Context mContext;

	public static Uri PREFERRED_APN_URI 					= Uri .parse("content://telephony/carriers/preferapn");
	
	public static final String CTWAP 						= "ctwap";
	public static final String CMWAP			 			= "cmwap";
	public static final String WAP_3G 						= "3gwap";
	public static final String UNIWAP 						= "uniwap";

	public static final int TYPE_NET_WORK_DISABLED 			= 0;	// The network is not available
	public static final int TYPE_CM_CU_WAP 					= 4;	// Mobile or  Unicom wap10.0.0.172
	public static final int TYPE_CT_WAP						= 5;	// Telecom WAP 10.0.0.200
	public static final int TYPE_OTHER_NET 					= 6;	// Telecom, mobile, Unicom, WiFi net network

	/**
	 * 
	* <p>Title: checkNetworkType</p>
	* <p>Description: Judge Network specific type (Unicom mobile telecommunications WAP, WAP, net)</p>
	* @param mContext
	* @return
	 */
	public static int checkNetworkType(Context mContext) {
		try {
			ConnectivityManager connectivityManager = (ConnectivityManager) mContext .getSystemService(Context.CONNECTIVITY_SERVICE);
			NetworkInfo mobNetInfoActivity = connectivityManager .getActiveNetworkInfo();
			if (mobNetInfoActivity == null || !mobNetInfoActivity.isAvailable()) {

				// note:
				// NetworkInfo is empty or not can be used normally it is not currently available network,
				// but some electrical machine, can still normal network,
				// so as net network processing still attempt to connect to the network.

				Log4Util.i(Device.TAG, "The network connection is not currently available.");
				return TYPE_NET_WORK_DISABLED;
			} else {

				// NetworkInfo is not null to judge is the network type
				int netType = mobNetInfoActivity.getType();
				if (netType == ConnectivityManager.TYPE_WIFI) {
					// wifi net
					Log4Util.i(Device.TAG, "The current network of type WIFI.");
					return TYPE_OTHER_NET;
				} else if (netType == ConnectivityManager.TYPE_MOBILE) {

					 // to judge whether the telecom wap:
					 // don't access getExtraInfo gets the access point 
					 //Because the current telecom types / testing found that the access point name mostly #777 or null,
					 // electrical machine WAP access point than Unicom mobile wap access points to set up a user name and password,
					 // we can judge by this!
					final Cursor c = mContext.getContentResolver().query(
							PREFERRED_APN_URI, null, null, null, null);
					if (c != null) {
						c.moveToFirst();
						final String user = c.getString(c.getColumnIndex("user"));
						if (!TextUtils.isEmpty(user)) {
							Log4Util.i(Device.TAG, " Internet Agents ��" + c.getString(c .getColumnIndex("proxy")));
							if (user.startsWith(CTWAP)) {
								Log4Util.i(Device.TAG, "The current network of type : Telecom WAP network ");
								return TYPE_CT_WAP;
							}
						}
					}
					c.close();

					// Note three:
					// judge  network of type is mobile  or Unicom wap
					// There is also a method by getString (c.getColumnIndex ("proxy") to obtain proxy IP
					// To determine the access point, 10.0.0.172's mobile Unicom WAP, 10.0.0.200 Telecom wap,
					// I can get to the access point information agent in the actual development and not all machines but,
					// like Meizu M9 (2.2)...
					// So the getExtraInfo is applied to get the access point name to judge

					String netMode = mobNetInfoActivity.getExtraInfo();
					Log4Util.i(Device.TAG, "The current network netMode: " + netMode);
					if (netMode != null) {
						// The APN name to determine whether it is China Mobile and China Unicom wap
						netMode = netMode.toLowerCase();
						if (netMode.equals(CMWAP) || netMode.equals(WAP_3G)
								|| netMode.equals(UNIWAP)) {
							Log4Util.i(Device.TAG, "The current network of type ,China Mobile or China Unicom wap");
							return TYPE_CM_CU_WAP;
						}

					}

				}
			}
		} catch (Exception ex) {
			ex.printStackTrace();
			return TYPE_OTHER_NET;
		}

		return TYPE_OTHER_NET;

	}
	
	
	public static APN checkNetworkAPNType(Context mContext) {
		APN apn = APN.UNKNOWN;
		try {
			ConnectivityManager connectivityManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
			NetworkInfo mobNetInfoActivity = connectivityManager .getActiveNetworkInfo();
			if (mobNetInfoActivity.getType() == ConnectivityManager.TYPE_WIFI) {
				apn = APN.WIFI;
			} else if (mobNetInfoActivity.getType() == ConnectivityManager.TYPE_MOBILE) {
				apn = _checkNetworkType(mobNetInfoActivity);
				
				// The following code will be bug when 3G Unicom and 2G switching,
				// When the 3G is switched to 2G, netInfo.getSubtypeName () returns the type is 3gnet, 
				// so the judge network type is 3G ,but the actual network for 2G
				
			}
		} catch (Exception e) {
			e.printStackTrace();
			apn = APN.UNKNOWN;
		}
		return apn;
	}
	
	/**
	 * 
	* <p>Title: isChinaMobileUnicomwap</p>
	* <p>Description: </p>
	* @param mContext
	* @return
	 */
	public static boolean isChinaMobileUnicomWap(Context mContext) {
		if(CheckApnTypeUtils.checkNetworkAPNType(mContext) == APN.WOWAP) {
			if(CheckApnTypeUtils.checkNetworkType(mContext) == CheckApnTypeUtils.TYPE_CM_CU_WAP
					|| CheckApnTypeUtils.checkNetworkType(mContext) == CheckApnTypeUtils.TYPE_CT_WAP) {
				return true;
			}
		}
		return false;
	}
	
	public static boolean _isChinaMobileUnicomWap(Context context) {
		int mobileNetWorkType = getMobileNetWorkType(context);
		if(mobileNetWorkType == 5 || mobileNetWorkType == 2
				|| mobileNetWorkType == 3) {
			return true;
		}
		return false;
	}
	
	
	public static int getMobileNetWorkType(Context context) {
		ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
		if(cm == null) {
			return -1;
		}
		NetworkInfo networkInfo = cm.getActiveNetworkInfo();
		if (networkInfo == null) {
			return -1;
		}
		if (networkInfo.getType() == 1) {
			return 0;
		}
		if (networkInfo.getExtraInfo() != null) {
			if (networkInfo.getExtraInfo().equalsIgnoreCase("uninet")) {
				return 1;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("uniwap")) {
				return 2;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("3gwap")) {
				return 3;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("3gnet")) {
				return 4;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("cmwap")) {
				return 5;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("cmnet")) {
				return 6;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("ctwap")) {
				return 7;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("ctnet")) {
				return 8;
			}
			if (networkInfo.getExtraInfo().equalsIgnoreCase("LTE")) {
				return 10;
			}
		}
		return 9;
	}
	
	/**
	 * Function: Check Network Type
	 * 
	 * @param context
	 * @return
	 */
	private static APN _checkNetworkType(NetworkInfo info) {
		try {
			if (info != null && info.isAvailable()
					&& info.getState() == NetworkInfo.State.CONNECTED) {
				switch (info.getType()) {
				case ConnectivityManager.TYPE_WIFI:
					return APN.WIFI;
				case ConnectivityManager.TYPE_MOBILE:
					switch (info.getSubtype()) {
					case TelephonyManager.NETWORK_TYPE_1xRTT:
					case TelephonyManager.NETWORK_TYPE_CDMA:
					case TelephonyManager.NETWORK_TYPE_EDGE:
					case TelephonyManager.NETWORK_TYPE_GPRS:
						return APN.WOWAP; // 2G ~ 100 kbps
					case TelephonyManager.NETWORK_TYPE_EVDO_0:
					case TelephonyManager.NETWORK_TYPE_EVDO_A:
					case TelephonyManager.NETWORK_TYPE_EVDO_B:
					case TelephonyManager.NETWORK_TYPE_HSDPA:
					case TelephonyManager.NETWORK_TYPE_HSPA:
					case TelephonyManager.NETWORK_TYPE_HSUPA:
					case TelephonyManager.NETWORK_TYPE_UMTS:
						return APN.WONET; // 3G ~ 400-7000 kbps
					case TelephonyManager.NETWORK_TYPE_UNKNOWN:
						return APN.UNKNOWN;
					}
				case ConnectivityManager.TYPE_MOBILE_DUN:
				case ConnectivityManager.TYPE_MOBILE_HIPRI:
				case ConnectivityManager.TYPE_MOBILE_MMS:
				case ConnectivityManager.TYPE_MOBILE_SUPL:
				case ConnectivityManager.TYPE_WIMAX:
					return APN.INTERNET;
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			return APN.UNKNOWN;
		}
		return APN.UNKNOWN;
	}
	
	
	 /**
     * Get the current APN and returns a {@link HttpHost} object
     * 
     * @return {@link HttpHost} object
     */
    public static HttpHost getHttpHost() {
        HttpHost proxy = null;
        Cursor mCursor = null;
        if (null != mContext) {
            mCursor = mContext.getContentResolver().query(PREFERRED_APN_URI, null, null, null, null);
        }
        if (mCursor != null && mCursor.moveToFirst()) {
        	// 游标移至第一条记录，当然也只有一条
            String proxyStr = mCursor.getString(mCursor.getColumnIndex("proxy"));
            if (proxyStr != null && proxyStr.trim().length() > 0) {
                proxy = new HttpHost(proxyStr, 80);
            }
            mCursor.close();
        }
        return proxy;
    }

}
