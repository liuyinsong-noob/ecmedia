/**
 * 
 */
package com.hisun.phone.core.voice;

import java.util.ArrayList;
import java.util.List;

import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.DeviceListener.*;
import com.hisun.phone.core.voice.multimedia.AudioRecordManager;
import com.hisun.phone.core.voice.multimedia.MediaManager;
import com.hisun.phone.core.voice.multimedia.MediaPlayManager;
import com.hisun.phone.core.voice.multimedia.RecordManager;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.IBinder;
import android.os.Parcelable;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;

/**
 * CCP service of voice.
 * 
 */
public class CCPService extends Service {

	public static final String CONNECT_ACTION = "android.net.conn.CONNECTIVITY_CHANGE";
	public static final String CHECK_ACTION = "com.ccp.phone.intent.CHECK_ATTIME_ACTION";
	public static final String ACTION_DEVICES = "action_devices";

	public static final String PHONE_OUTGOING_ACTION = "android.intent.action.NEW_OUTGOING_CALL";

	private final CCPCallBinder binder = new CCPCallBinder();
	private CallControlManager callControlManager;
	private Exception callManagerError;
	private MediaManager mediaManager;
	private AlarmManager alarmManager;
	private InnerCoreReceiver receiver;
	private TelephonyManager mTelephonyManager;
	private PhoneStateListener mPhoneStateListener;
	private ConnectivityManager mConnManager;
	
	private int lastStartId = -1;

	private volatile APN apn;
	private volatile NetworkState networkState;

	private int curNetworkType = -1;
	private int lastConnectedNetworkType = -1;
	private String lastWifiSSID;
	private long lastDisconnectTime = 0L;
	
	@Override
	public void onCreate() {
		super.onCreate();

		// initial low library at first
		NativeInterface.init();
		
		alarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
		mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
		mConnManager = (ConnectivityManager) getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
		
		// register network receiver
		IntentFilter filter = new IntentFilter(CONNECT_ACTION);
		filter.addAction(CHECK_ACTION);
		receiver = new InnerCoreReceiver();
		registerReceiver(receiver, filter);

		callControlManager = CallControlManager.initialize(getApplicationContext());
		mediaManager = MediaManager.initialize(getApplicationContext());
		
		mPhoneStateListener = new PhoneStateListener() {

			@Override
			public void onCallStateChanged(int state, String incomingNumber) {
				Log4Util.d("onCallStateChanged: " + state + ",IncomingNumber: " + incomingNumber);
				switch (state) {
				case TelephonyManager.CALL_STATE_IDLE:
					Log4Util.i(Device.TAG, "system call idle.");
					break;
				case TelephonyManager.CALL_STATE_RINGING:
				case TelephonyManager.CALL_STATE_OFFHOOK:
					Log4Util.i(Device.TAG, "handle voip call when system call offhook.");
					
					callControlManager.postCommand(new Runnable() {

						@Override
						public void run() {
							try {
								// notify device apn changed
								List<Device> devices = CCPCallImpl.getInstance().listDevices();
								for (int i = 0; i < devices.size(); i++) {
									DeviceImpl device = (DeviceImpl) devices.get(i);
									
									// recording ,voice play,VoIP
									if (device != null && (device.isKeepingCall() || device.isVoicePlaying() || device.isVoiceRecording())) {
										Log4Util.i(Device.TAG, "found system call coming, then notify apps. ThreadName: " + Thread.currentThread().getName());
										//device.releaseCall(device.getCurrentCall());
										device.getDeviceListener().onReceiveEvents(CCPEvents.SYSCallComing/*, apn, networkState*/);
									}
								}
							} catch (Exception e) {
								e.printStackTrace();
							}
						}
					});
					break;
				}
			}

		};
		mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
	}

	class InnerCoreReceiver extends BroadcastReceiver {

		public InnerCoreReceiver() {
			updateNetworkState();
			if (curNetworkType == -1) {
				lastDisconnectTime = System.currentTimeMillis();
			}
		}
		
		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent == null || context == null) {
				return;
			}
			
			final String action = intent.getAction() == null ? "" : intent.getAction();
			Log4Util.d(Device.TAG, "[CCPService - onReceive] action = " + action);
			
			if (action.equals(CONNECT_ACTION)) {
				handleNetChanged(intent);
			} else if (action.equals(CHECK_ACTION)) {
				handleCheckAction();
			}
		}
	}

	private boolean handleCheckAction() {
		if(VoiceUtil.getNetWorkType() == NETWORK_NONE || callControlManager == null) {
			return false;
		}
		if (callControlManager.checkNormalSoftAddress()) {
			cancelAlarmTime(CHECK_ACTION);
			Log4Util.i(Device.TAG, "soft switch address normal, so cancel check of alarm.");
			return true;
		}

		try {
			this.callControlManager.doQuerySoftSwitchAddress();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return false;
	}
	
	private void handleNetChanged(final Intent intent) {
		
		callControlManager.postCommand(new Runnable() {

			@Override
			public void run() {
				boolean haveConnection = !intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false);

				boolean wasConnected = (curNetworkType != -1);
				boolean netTypeChanged = updateNetworkState();
				Log4Util.d(Device.TAG, "[NeedReconnected] network state: " + haveConnection + ", connect type changed: " + netTypeChanged);
				
				if ((wasConnected) && (!haveConnection)) {
					Log4Util.d(Device.TAG, "this is type is 1.last connect 2.this have disconnect");
					lastDisconnectTime = System.currentTimeMillis();
					notifyApnEvents(transferNetType(-1), false, false);
				} else if ((!wasConnected) && (haveConnection) && (!netTypeChanged) && (System.currentTimeMillis() - lastDisconnectTime > 20000L)) {
					Log4Util.d(Device.TAG, "this is type is 1.last disconnect 2.this have connect 3.network not changed 4.interval over 20s.");
					notifyApnEvents(transferNetType(curNetworkType), haveConnection, true);
				} else if(!wasConnected && (haveConnection)) {
					Log4Util.d(Device.TAG, "this is type is 1.last disconnect 2.this have connect");
					notifyApnEvents(transferNetType(curNetworkType), haveConnection, true);
				} else if (netTypeChanged) {
					Log4Util.d(Device.TAG, "this is type is 1.network changed.");
					notifyApnEvents(transferNetType(curNetworkType), haveConnection, true);
				} else if (haveConnection && !netTypeChanged && lastConnectedNetworkType == ConnectivityManager.TYPE_MOBILE) {
					Log4Util.d(Device.TAG, "this is type is 1.this have connect 2.network not changed 3.last network mobile.");
					notifyApnEvents(transferNetType(curNetworkType), haveConnection, true);
				}
			}
			
		});
	}
	
	public static final int NETWORK_NONE = 0;
	public static final int NETWORK_LAN = 1;
	public static final int NETWORK_WIFI = 2;
	public static final int NETWORK_GPRS = 3;
	public static final int NETWORK_3G = 4;
	
	private synchronized int transferNetType(final int localType) {
		int finalType = NETWORK_NONE;
		switch (localType) {
		case -1:
			finalType = NETWORK_NONE;
			break;
		case ConnectivityManager.TYPE_WIFI:
			finalType = NETWORK_WIFI;
			break;
		case ConnectivityManager.TYPE_MOBILE:
			if (apn == APN.WONET) {
				finalType = NETWORK_3G;
			} else {
				finalType = NETWORK_GPRS;
			}
			break;
		default:
			Log4Util.e("[transferNetType] found a new local net type: " + localType);
			break;
		}
		return finalType;
	}
	
	private void notifyApnEvents(int netType, boolean haveConnection, boolean reconnect) {
		try {
			Log4Util.d("[notifyApnEvents] tell so the network changed.");
			
			VoiceUtil.setNetWorkType(netType);
			
			if(haveConnection && reconnect) {
				if(handleCheckAction()) {
					// network changed, against request register
					callControlManager.setNetworkType(netType, haveConnection, reconnect);
				}
			}
			// notify device apn changed
			/*List<Device> devices = CCPCallImpl.getInstance().listDevices();
			for (int i = 0; i < devices.size(); i++) {
				Device device = devices.get(i);
				if (device != null && device.getDeviceListener() != null) {
					device.getDeviceListener().receiveEvents(CCPEvents.NSChanged, apn, networkState);
				}
			}*/
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private boolean updateNetworkState() {
		boolean changed = false;
		int oldNetworkType = this.lastConnectedNetworkType;

		NetworkInfo netInfo = mConnManager.getActiveNetworkInfo();
		if (netInfo != null) {
			this.lastConnectedNetworkType = (this.curNetworkType = netInfo.getType());
			
			if (netInfo.getState() == NetworkInfo.State.CONNECTING || 
					netInfo.getState() == NetworkInfo.State.CONNECTED) { 
				networkState = NetworkState.CONNECTED;
			} else if (netInfo.getState() == NetworkInfo.State.DISCONNECTED){
				networkState = NetworkState.DISCONNECTED;
			} else {
				networkState = NetworkState.UNKNOWN;
			}
			
			if (netInfo.getType() == ConnectivityManager.TYPE_WIFI) {
				apn = APN.WIFI;
			} else if (netInfo.getType() == ConnectivityManager.TYPE_MOBILE) {
				apn = checkNetworkType(netInfo);
				
				// The following code will be bug when 3G Unicom and 2G switching,
				// When the 3G is switched to 2G, netInfo.getSubtypeName () returns the type is 3gnet, 
				// so the judge network type is 3G ,but the actual network for 2G
				
				//checkApnType(netInfo);
			}
		} else {
			this.curNetworkType = -1;
			this.networkState = NetworkState.UNKNOWN;
			this.apn = APN.UNKNOWN;
		}
		
		changed = (this.curNetworkType != -1) && (this.lastConnectedNetworkType != oldNetworkType);
		
		if ((this.curNetworkType == ConnectivityManager.TYPE_WIFI) && (oldNetworkType == ConnectivityManager.TYPE_WIFI)) {
			WifiManager wifiMan = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
			WifiInfo wifiInfo = wifiMan.getConnectionInfo();
			if (wifiInfo != null) {
				String oldSSID = this.lastWifiSSID;
				this.lastWifiSSID = wifiInfo.getSSID();
				if ((this.lastWifiSSID == null) || (!this.lastWifiSSID.equals(oldSSID))) {
					changed = true;
				}
			} else {
				this.lastWifiSSID = null;
			}
		} else {
			this.lastWifiSSID = null;
		}

		return changed;
	}

	/**
	 * ��ȡ��ǰ������״̬��
	 * 
	 * @return WIFI/2G/3G
	 */
	public String getNetworkName() {
		if (apn == APN.WIFI) {
			return "WIFI";
		}
		if (apn == APN.CMWAP || apn == APN.CMNET || apn == APN.UNIWAP
				|| apn == APN.UNINET || apn == APN.CTWAP || apn == APN.CTNET
				|| apn == APN.INTERNET || apn == APN.GPRS) {
			return "2G";
		}
		return "3G";
	}

	@Override
	public void onDestroy() {
		Log4Util.e(Device.TAG, "[CCPService - onDestroy] CCPService destroy.");
		try {
			unregisterReceiver(receiver);
		} catch (Exception e) {
		}
		if (this.callControlManager != null) {
			this.callControlManager.destroy();
		}
		if (this.mediaManager != null) {
			this.mediaManager.destroy();
		}
		cancelAlarmTime(CHECK_ACTION);
		this.lastStartId = -1;
		apn = null;
		networkState = null;
		curNetworkType = -1;
		lastConnectedNetworkType = -1;
		lastWifiSSID = null;
		lastDisconnectTime = 0L;
		
		//CCPCall.shutdown();
		
		super.onDestroy();
	}

	@Override
	public IBinder onBind(Intent intent) {
		return this.binder;
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		if (intent == null) {
			return super.onStartCommand(intent, flags, startId);
		}

		String action = intent.getAction() == null ? "" : intent.getAction();
		Log4Util.d(Device.TAG, "[CCPService] onStart action=" + action + ", flags=" + flags + ", startId=" + startId);

		if ((flags & 0x1) != 0) {
			restoreState(intent, this.binder);
		}

		if (this.lastStartId != -1) {
			stopSelfResult(this.lastStartId);
		}
		
		this.lastStartId = startId;

		return START_REDELIVER_INTENT;
	}

	public void restoreState(Intent intent, CCPService.CCPCallBinder ccpBinder) {
		Log4Util.w(Device.TAG, "[CCPService - restoreState] restore state from CCPService.");
		
		CCPCallImpl ccpImpl = CCPCallImpl.getInstance();
		if ((ccpImpl.isInitializing()) || (ccpImpl.isInitialized())) {
			Log4Util.w(Device.TAG, "CCPImpl is initializing or initialized.");
			return;
		}

		ArrayList<Device> parcelable = intent.getParcelableArrayListExtra(Device.DEVICES_ARRAY);
		if ((parcelable == null) || (parcelable.isEmpty())) {
			return;
		}

		ArrayList<Device> devicesInfo = new ArrayList<Device>(parcelable.size());
		for (Parcelable p : parcelable) {
			if ((p instanceof Device)) {
				devicesInfo.add((Device) p);
			}
		}
		if (devicesInfo.isEmpty()) {
			return;
		}
		ccpImpl.reverseInitialize(getApplicationContext(), ccpBinder, devicesInfo);
	}

	@Override
	public boolean onUnbind(Intent intent) {
		Log4Util.e(Device.TAG, "[CCPService - onUnbind]");
		return super.onUnbind(intent);
	}

	class CCPCallBinder extends Binder {

		public CallControlManager getCallControlManager() {
			return CCPService.this.getCallControlManager();
		}

		public MediaManager getMediaManager() {
			return CCPService.this.getMediaManager();
		}

		public RecordManager getRecordManager() {
			return RecordManager.getInstance();
		}
		
		public AudioRecordManager getAudioRecordManager() {
			return AudioRecordManager.getInstance();
		}
		
		public MediaPlayManager getMediaPlayManager() {
			return MediaPlayManager.getInstance();
		}
		
		public Exception getError() {
			return CCPService.this.getCallManagerError();
		}

		public NetworkState getNetworkState() {
			return networkState;
		}

		public APN getAPN() {
			return apn;
		}
		
		public void setAlarmTime() {
			CCPService.this.setAlarmTime(System.currentTimeMillis(), 15000, CHECK_ACTION);
		}
	}

	private void setAlarmTime(long triggerAtTime, long interval, String action) {
        PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(action), PendingIntent.FLAG_CANCEL_CURRENT);
        alarmManager.setRepeating(AlarmManager.RTC_WAKEUP, triggerAtTime, interval , pendingIntent);
	}
	
	private void cancelAlarmTime(String action) {
		PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, new Intent(action), PendingIntent.FLAG_CANCEL_CURRENT);
		alarmManager.cancel(pendingIntent);
	}
	
	/**
	 * @return the callManager
	 */
	public CallControlManager getCallControlManager() {
		return callControlManager;
	}

	/**
	 * @return the callManagerError
	 */
	public Exception getCallManagerError() {
		return callManagerError;
	}

	/**
	 * @return the mediaManager
	 */
	public MediaManager getMediaManager() {
		return mediaManager;
	}
	
	/**
	 * Function: Check Network Type
	 * 
	 * @param context
	 * @return
	 */
	private APN checkNetworkType(NetworkInfo info) {
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

}
