package com.hisun.phone.core.voice.util;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Random;
import java.util.TimeZone;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.CCPService;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.DeviceListener.APN;
import com.hisun.phone.core.voice.model.CCPParameters;
import com.hisun.phone.core.voice.model.NetworkStatistic;

/**O
 * 
 */
public final class VoiceUtil {

	public static final String CCP_DEFAULT_PACKAGE_NAME = "com.ccp.phone";
	public static final SimpleDateFormat TimestampFormat = new SimpleDateFormat("yyyyMMddHHmmss");
	public static final TimeZone tz = TimeZone.getTimeZone("GMT+8:00");
	public static final Random rnd = new Random();
	private static String deviceNo = null;
	
	private static int mNetworkType = CCPService.NETWORK_NONE;
	
	private static final HashMap<String, String> sVersion = new HashMap<String, String>();
	
	public static final String XMLHeader = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\t\n";
	
	
	private static String property = System.getProperty("line.separator");
	
	static {
		TimestampFormat.setTimeZone(tz);
	}

	public static int creatRandom(int t) {
		return Math.abs(rnd.nextInt(t));
	}
	
	/**
	 * 
	 * @return
	 */
	public static String getPropertyLine() {
		return property;
	}

	private VoiceUtil() {

	}

	public static String getRandomDeviceNo() {
		return md5(VoiceUtil.formatTimestamp(System.currentTimeMillis()) + VoiceUtil.getRandomNumber(6));
	}
	
	public static String getIMEI(Context context) {
		TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
		
		if(tm == null || TextUtils.isEmpty(tm.getDeviceId())) {
			return "";
		}
		
		return tm.getDeviceId();
	}
	
	public static String getLocalMacAddressMd5(Context context) {
		return md5(getLocalMacAddress(context));
	}
	
	/**
	 * 
	 * @param context
	 * @return
	 */
	public static String getLocalMacAddress(Context context) {
		WifiManager wifi = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
		
		if(wifi == null || wifi.getConnectionInfo() == null || TextUtils.isEmpty(wifi.getConnectionInfo().getMacAddress())) {
			return "";
		}
		
		return wifi.getConnectionInfo().getMacAddress();
	}
	
	public static String getLastwords(String srcText, String p) {
		try {
			String[] array = TextUtils.split(srcText, p);
			int index = (array.length - 1 < 0) ? 0 : array.length - 1;
			return array[index];
		} catch (Exception e) {
			e.printStackTrace();
		}
		return null;
	}

	public static String[] split(String original, String regex) {
		return split(original, regex, false);
	}

	public static String[] split(String original, String regex,
			boolean isTogether) {
		try {
			int startIndex = original.indexOf(regex);
			int index = 0;
			if (startIndex < 0) {
				return new String[] { original };
			}
			ArrayList<String> v = new ArrayList<String>();
			while (startIndex < original.length() && startIndex != -1) {
				String temp = original.substring(index, startIndex);
				v.add(temp);
				index = startIndex + regex.length();
				startIndex = original.indexOf(regex, startIndex
						+ regex.length());
			}
			if (original.indexOf(regex, original.length() - regex.length()) < 0) {
				String last = original.substring(index);
				if (isTogether) {
					last = regex + last;
				}
				v.add(last);
			}

			return v.toArray(new String[v.size()]);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
	}

	private static MessageDigest md = null;

	public static String md5(final String c) {
		if (md == null) {
			try {
				md = MessageDigest.getInstance("MD5");
			} catch (NoSuchAlgorithmException e) {
				e.printStackTrace();
			}
		}

		if (md != null) {
			md.update(c.getBytes());
			return byte2hex(md.digest());
		}
		return "";
	}

	public static String byte2hex(byte b[]) {
		String hs = "";
		String stmp = "";
		for (int n = 0; n < b.length; n++) {
			stmp = Integer.toHexString(b[n] & 0xff);
			if (stmp.length() == 1)
				hs = hs + "0" + stmp;
			else
				hs = hs + stmp;
			if (n < b.length - 1)
				hs = (new StringBuffer(String.valueOf(hs))).toString();
		}

		return hs.toUpperCase();
	}
	
	public static String formatTimestamp(long time) {
		return TimestampFormat.format(new Date(time));
	}
	
	public static boolean checkIPAddr(String ip) {
		if (ip == null || ip.length() == 0) {
			return false;
		}
		Pattern pattern = Pattern
				.compile("\\b((?!\\d\\d\\d)\\d+|1\\d\\d|2[0-4]\\d|25[0-5])\\.((?!\\d\\d\\d)\\d+|1\\d\\d|2[0-4]\\d|25[0-5])\\.((?!\\d\\d\\d)\\d+|1\\d\\d|2[0-4]\\d|25[0-5])\\.((?!\\d\\d\\d)\\d+|1\\d\\d|2[0-4]\\d|25[0-5])\\b");
		Matcher matcher = pattern.matcher(ip);
		return matcher.matches();
	}
	
	public static String getStandardMDN(String number) {
		if (number == null || number.length() == 0) {
			return null;
		}

		if (number.startsWith("0086") || number.startsWith("+86")) {
			return number;
		} else {
			return "0086" + number;
		}
	}
	
	/**
	 * @param number
	 * @return
	 */
	public static boolean isP2LCallNumnber(String number) {
		
		if(TextUtils.isEmpty(number)) {
			number = "";
		}
		
		if(number.startsWith("0086") || number.startsWith("+86")) {
			return true;
		}
		
		return false;
	}
	
	// Get 6 numbers and the composition string.
	public static String getRandomNumber(int length) {
		String val = "";
		Random random = new Random();
		for (int i = 0; i < length; i++) {
			val += String.valueOf(random.nextInt(10));
		}

		return val;
	}
	
	

	/**
	 * Java文件操作 获取文件扩展名
	 * Get the file extension, if no extension or file name
	 * 
	 */
	public static String getExtensionName(String filename) {
		if ((filename != null) && (filename.length() > 0)) {
			int dot = filename.lastIndexOf('.');
			if ((dot > -1) && (dot < (filename.length() - 1))) {
				return filename.substring(dot + 1);
			}
		}
		return filename;
	}

	/**
	 * Java文件操作 获取不带扩展名的文件名
	 */
	public static String getFileNameNoEx(String filename) {
		if ((filename != null) && (filename.length() > 0)) {
			int dot = filename.lastIndexOf('.');
			if ((dot > -1) && (dot < (filename.length()))) {
				return filename.substring(0, dot);
			}
		}
		return filename;
	}
	
	/**
	 * IMSI号前面3位460是国家，紧接着后面2位00 02是中国移动，01是中国联通，03是中国电信
	 * 
	 * @return
	 */
	public static SIMProvider getSIMProvider(Context context) {
		try {
			SIMProvider providersName = SIMProvider.UNKNOWN;
			TelephonyManager mTelephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
			String imsi = mTelephonyManager.getSubscriberId();
			if (imsi.startsWith("46000") || imsi.startsWith("46002")) {
				providersName = SIMProvider.CMCC;
			} else if (imsi.startsWith("46001")) {
				providersName = SIMProvider.UNICOM;
			} else if (imsi.startsWith("46003")) {
				providersName = SIMProvider.TELECOM;
			}
			Log4Util.i(Device.TAG, "SIM Provider is: " + providersName.name());
			return providersName;
		} catch (Exception e) {
			//e.printStackTrace();
			Log4Util.i(Device.TAG, "VoiceUtil.getSIMProvider " + e.getLocalizedMessage());
		}
		
		return SIMProvider.UNKNOWN;
	}
	
	public static enum SIMProvider {
		CMCC, UNICOM, TELECOM, UNKNOWN
	}
	
	/**
	 * 2013/04/17 获取软交换地址增加设备唯一号参数
	 * 
	 * @param context
	 * @return
	 */
	public static String createDeviceNo(Context context) {
		/*SharedPreferences sPreferences = context.getSharedPreferences(
				"sdk_preference", Context.MODE_PRIVATE);
		String deviceNo = sPreferences.getString("sdk_deviceNo", "");

		if (TextUtils.isEmpty(deviceNo)) {
			TelephonyManager telephonyManager = (TelephonyManager) context
					.getSystemService(Context.TELEPHONY_SERVICE);
			String deviceId = telephonyManager.getDeviceId();
			if (TextUtils.isEmpty(deviceId)) {
				return null;
			}
			deviceNo = VoiceUtil.md5(deviceId + VoiceUtil.getDigitNumr(6)
					+ VoiceUtil.formatTimestamp(System.currentTimeMillis()));

			Editor edit = sPreferences.edit();
			edit.putString("sdk_deviceNo", deviceNo);
			edit.commit();
		}*/
		
		try {
			if (deviceNo == null) {
				deviceNo = getLocalMacAddressMd5(context);
			}
			return deviceNo;
		} catch (Exception e) {
			e.printStackTrace();
			return VoiceUtil.getRandomDeviceNo();
		}
	}
	
	/**
	 * Get the text message time
	 * @param time
	 * @return
	 */
	public static String getTextReceiveDate(String time) {
		// 2012 05 17 18 49 03
		StringBuffer sbBuffer = new StringBuffer();
		sbBuffer.append(time.substring(0, 4)+"-");
		sbBuffer.append(time.substring(4, 6)+"-");
		sbBuffer.append(time.substring(6, 8)+" ");

		sbBuffer.append(time.substring(8, 10)+":");
		sbBuffer.append(time.substring(10, 12)+":");
		sbBuffer.append(time.substring(12, time.length()));

		return sbBuffer.toString();
	}
	
	/**
	 * 
	 * @param context
	 * @return
	 */
	public static String getPackageName(Context context) {
		String packageName = null;
		try {
			PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
		    // ��ǰ�汾�İ���
			packageName =  info.packageName;
		    
		} catch (Exception e) {
		    e.printStackTrace();
		}
		if(TextUtils.isEmpty(packageName)) {
			packageName = CCP_DEFAULT_PACKAGE_NAME;
		}
		return packageName;
	}
	
	/**
	 * 
	 * @param version
	 * @return
	 */
	public static boolean isHeighVersion(int version) {
		if(Build.VERSION.SDK_INT < version) {
			return false;
		}
		return true;
	}
	
	public static String getDate(long time) {
		try {
			SimpleDateFormat simpleFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			Calendar c = Calendar.getInstance();
			c.setTimeInMillis(time);
			return simpleFormat.format(c.getTime());
		} catch (Exception e) {
			e.printStackTrace();
		}
		return "";
	}
	
	/**
	 * 
	* <p>Title: isAvaiableSpace</p>
	* <p>Description: The current sdcard of free space</p>
	* @param sizeMb
	* @return
	 */
	public static boolean isAvaiableSpace(int sizeMb) {
		if (android.os.Environment.getExternalStorageState().equals(
				android.os.Environment.MEDIA_MOUNTED)) {
			String sdcard = Environment.getExternalStorageDirectory().getPath();
			StatFs statFs = new StatFs(sdcard);
			long blockSize = statFs.getBlockSize();
			long blocks = statFs.getAvailableBlocks();
			long availableSpare = (blocks * blockSize) / (1024 * 1024);
			Log4Util.v(Device.TAG, "The current sdcard of free space :" + availableSpare);
			if (sizeMb > availableSpare) {
				return false;
			} else {
				return true;
			}
		}
		Log4Util.v(Device.TAG, "The current is not loaded sdcard" );
		return false;
	}
	
	/**
	 * 
	* <p>Title: checkNetworkAPNType</p>
	* <p>Description: </p>
	* @param context
	* @return
	 */
	public static APN checkNetworkAPNType(Context context) {
		return CheckApnTypeUtils.checkNetworkAPNType(context);
	}
	
	public static void setNetWorkType(int type) {
		mNetworkType = type;
	}
	
	public static boolean isNetWorkTypeWIFI() {
		return mNetworkType == CCPService.NETWORK_WIFI;
	}
	
	public static int getNetWorkType() {
		return mNetworkType;
	}
	
	/**
	 * 
	 * @return
	 */
	public static boolean isExistExternalStore() {
		if (Environment.getExternalStorageState().equals(
				Environment.MEDIA_MOUNTED)) {
			return true;
		} else {
			return false;
		}
	}
	
	/**
	 * 
	 * @return
	 */
	public static String getExternalStorePath() {
		if (isExistExternalStore()) {
			return Environment.getExternalStorageDirectory().getAbsolutePath();
		}
		return null;
	}
	
	/*
	* Convert String to InputString using ByteArrayInputStream class.
	* This class constructor takes the string byte array which can be
	* done by calling the getBytes() method.
	*/
	public static InputStream string2InputStream(String str) {
		
		if(TextUtils.isEmpty(str)) {
			return null;
		}
		
		try {
			return new ByteArrayInputStream(str.getBytes("UTF-8"));
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		}
		
		return null;
	}
	
	/**
	 * 
	 * @param parameters
	 * @return
	 */
	public static String encodeUrl(CCPParameters parameters) {
		if (parameters == null) {
			return "";
		}

		StringBuilder sb = new StringBuilder();
		boolean first = true;
		for (int loc = 0; loc < parameters.size(); loc++) {
			
			String _key = parameters.getKey(loc);
			if("Authorization".equals(_key)) {
				continue;
			}
			
			if (first) {
				first = false;
			} else {
				sb.append("&");
			}
			
			String _value = parameters.getValue(_key);
			
			if (_value == null)
				Log4Util.i(Device.TAG, "VoiceUtil.encodeUrl key:" + _key + " 's value is null");
			else {
				try {
					sb.append(URLEncoder.encode(parameters.getKey(loc), "UTF-8")
							+ "="
							+ URLEncoder.encode(parameters.getValue(loc),
									"UTF-8"));
				} catch (UnsupportedEncodingException e) {
					e.printStackTrace();
				}
			}
			Log4Util.i(Device.TAG, "VoiceUtil.encodeUrl : " + sb.toString());
		}
		return sb.toString();
	}
	
	/**
	 * Format the Http request parameters
	 * @param params
	 * @return
	 */
	public static String encodeParameters(CCPParameters params) {
		if ((params == null) || (isBundleEmpty(params))) {
			return "";
		}
		StringBuilder buf = new StringBuilder();
		int j = 0;
		for (int loc = 0; loc < params.size(); loc++) {
			String key = params.getKey(loc);
			if (j != 0)
				buf.append("&");
			try {
				buf.append(URLEncoder.encode(key, "UTF-8"))
						.append("=")
						.append(URLEncoder.encode(params.getValue(key),
								"UTF-8"));
			} catch (UnsupportedEncodingException e) {
				// 
			}
			j++;
		}
		return buf.toString();
	}
	
	/**
	 * 
	 * @param params
	 * @return
	 */
	public static String buildRequestBody(CCPParameters params) {
		if ((params == null) 
				|| (isBundleEmpty(params)) 
				|| TextUtils.isEmpty(params.getParamerTagKey())) {
			return "";
		}
		StringBuilder buf = new StringBuilder();
		if((!"2013-03-22".equals(com.hisun.phone.core.voice.Build.REST_VERSION))) {
			buf.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\t\n");
		}
		buf.append("<" + params.getParamerTagKey() + ">\t\n");
		for (int loc = 0; loc < params.size(); loc++) {
			
			String key = params.getKey(loc);
			if("Authorization".equals(key)) {
				continue;
			}
			
			if(!TextUtils.isEmpty(key)&& key.equals(params.getParamerTagKey())) {
				buf.append(params.getValue(key));
				continue;
			}
			buf.append("\t<" + key + ">").append(params.getValue(key))
					.append("</" + key + ">\t\n");
		}
		buf.append("</" +params.getParamerTagKey() +  ">\t\n");
		String requestBody = buf.toString();
		Log4Util.i(Device.TAG, "doRequestUrl SDK request Body :\t\n" + requestBody);
		return requestBody;
	}

	private static boolean isBundleEmpty(CCPParameters bundle) {
	    if ((bundle == null) || (bundle.size() == 0)) {
	      return true;
	    }
	    return false;
	  }
	
	
	public static NetworkStatistic parserTrafficStats(String trafficStats) {
		try {
			int _duration = trafficStats.indexOf("<duration>");
			int _duration_ext = trafficStats.indexOf("</duration>");
			int _send = trafficStats.indexOf("<sendwifi>");
			int _send_ext = trafficStats.indexOf("</sendwifi>");
			int _recv = trafficStats.indexOf("<recvwifi>");
			int _recv_ext = trafficStats.indexOf("</recvwifi>");
			
			String duration = trafficStats.substring(_duration + 10, _duration_ext);
			String txBytes = trafficStats.substring(_send + 10, _send_ext);
			String rxBytes = trafficStats.substring(_recv + 10, _recv_ext);
			Log4Util.d(Device.TAG, "call trafficStats duration:" + duration + " , txBytes :" + txBytes + " , rxBytes:" + rxBytes);
			return new NetworkStatistic(Long.parseLong(duration) ,Long.parseLong(txBytes) , Long.parseLong(rxBytes));
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		return new NetworkStatistic(0,0,0);
	}
	
	static void getSDKVersion() {
		String _version = getSoFullVersion();
		String[] _versions = _version.split("#");
		if(_versions == null) {
			return;
		}
			
		sVersion.put("version" , _versions[0]);
		sVersion.put("platform" , _versions[1]);
		sVersion.put("ARMVersion" , _versions[2]);
		if(_versions[3].startsWith("voice")) {
			sVersion.put("audioSwitch" , _versions[3].split("=")[1]);
		}
		if (_versions[4].startsWith("video")) {
			sVersion.put("videoSwitch" , _versions[4].split("=")[1]);
		}
		sVersion.put("compileDate" , _versions[5]);
		
		sVersion.put("full", _version);
	}
	
	public static String getVersoinString(String prefix) {
		if(sVersion.isEmpty()) {
			getSDKVersion();
		}
		
		return sVersion.get(prefix);
	}
	
	static String getSoFullVersion() {
		return NativeInterface.getVersion();
	}
}
