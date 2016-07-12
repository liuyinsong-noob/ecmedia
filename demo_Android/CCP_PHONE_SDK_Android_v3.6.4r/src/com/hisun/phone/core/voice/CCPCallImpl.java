/**
 * 
 */
package com.hisun.phone.core.voice;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.os.Build.VERSION;
import android.os.IBinder;

import com.hisun.phone.core.voice.CCPCall.InitListener;
import com.hisun.phone.core.voice.DeviceListener.APN;
import com.hisun.phone.core.voice.DeviceListener.NetworkState;
import com.hisun.phone.core.voice.multimedia.AudioRecordManager;
import com.hisun.phone.core.voice.multimedia.MediaManager;
import com.hisun.phone.core.voice.multimedia.MediaPlayManager;
import com.hisun.phone.core.voice.multimedia.RecordManager;
import com.hisun.phone.core.voice.util.Log4Util;

/**
 * CCPCall implemention.
 * 
 */
public final class CCPCallImpl {
	private static CCPCallImpl instance;
	private boolean sdkIniting;
	private boolean sdkInited;
	private static final String[] RequiredPermissions = {
			"android.permission.INTERNET", "android.permission.RECORD_AUDIO",
			"android.permission.MODIFY_AUDIO_SETTINGS",
			"android.permission.ACCESS_NETWORK_STATE",
			"android.permission.ACCESS_WIFI_STATE",
			"android.permission.READ_PHONE_STATE",
			"android.permission.WAKE_LOCK",
			"android.permission.DISABLE_KEYGUARD",
			"android.permission.WRITE_EXTERNAL_STORAGE",
			"android.permission.CAMERA", 
			"android.permission.VIBRATE"};

	private Context context;
	private ServiceConnection serviceConn;
	protected CCPService.CCPCallBinder twBinder;
	protected CallControlManager callControlManager;
	protected MediaManager mediaManager;
	protected RecordManager recordManager;
	protected AudioRecordManager audRecordManager;
	protected MediaPlayManager mediaPlayManager;
	protected final HashMap<UUID, DeviceImpl> devices = new HashMap<UUID, DeviceImpl>();
	
	private CCPCallImpl() {

	}

	public static CCPCallImpl getInstance() {
		if (instance == null) {
			instance = new CCPCallImpl();
		}
		return instance;
	}

	boolean isInitialized() {
		return this.sdkInited;
	}

	boolean isInitializing() {
		return this.sdkIniting;
	}

	void init(Context inContext, final InitListener initListener) {
		if (isInitialized() || isInitializing()) {
			initListener.onError(new RuntimeException("CCPCall.init() already called."));
			return;
		}

		this.sdkIniting = true;
		try {
			PackageManager pm = inContext.getPackageManager();
			PackageInfo pinfo = pm.getPackageInfo(inContext.getPackageName(), PackageManager.GET_SERVICES
							| PackageManager.GET_PERMISSIONS);

			int sdkVersion = VERSION.SDK_INT;
			if( sdkVersion < 8 ) {
				throw new RuntimeException("CCP SDK supports the minimum version 8, the current version : " + sdkVersion);
			}
			
			HashMap<String, Boolean> appPermissions = new HashMap<String, Boolean>(
					pinfo.requestedPermissions != null ? pinfo.requestedPermissions.length : 0);
			
			
			if (pinfo.requestedPermissions != null) {
				for (String permission : pinfo.requestedPermissions) {
					appPermissions.put(permission, Boolean.valueOf(true));
				}
			}
			List<String> missingPermissions = new LinkedList<String>();
			for (String permission : RequiredPermissions) {
				if (!appPermissions.containsKey(permission)) {
					missingPermissions.add(permission);
				}
			}

			if (!missingPermissions.isEmpty()) {
				StringBuilder builder = new StringBuilder("Your app is missing the following required permissions:");
				for (String permission : missingPermissions) {
					builder.append(' ').append(permission);
				}
				throw new RuntimeException(builder.toString());
			}

			boolean serviceFound = false;
			if (pinfo.services != null) {
				for (ServiceInfo service : pinfo.services) {
					if (!service.name.equals(CCPService.class.getName())) {
						continue;
					}
					serviceFound = true;
					if (service.exported) {
						throw new RuntimeException("CCPService is exported. You must add android:exported=\"false\" to the <service> declaration in AndroidManifest.xml.");
					}
				}
			}

			if (!serviceFound) {
				throw new RuntimeException("com.hisun.phone.core.voice.CCPService is not declared in AndroidManifest.xml.");
			}
		} catch (Exception e) {
			e.printStackTrace();
			initListener.onError(e);
			this.sdkIniting = false;
			this.sdkInited = false;
			return;
		}

		this.context = inContext;
		final Intent service = new Intent(this.context, CCPService.class);
		
		this.serviceConn = new ServiceConnection() {

			@Override
			public void onServiceConnected(ComponentName name, IBinder binder) {
				Log4Util.w(Device.TAG, "[CCPCallImpl - onServiceConnected] " + Thread.currentThread().getName());
				sdkIniting = false;
				sdkInited = true;
				
				CCPCallImpl.this.context.startService(service);
				
				CCPCallImpl.this.twBinder = ((CCPService.CCPCallBinder) binder);
				CCPCallImpl.this.callControlManager = CCPCallImpl.this.twBinder.getCallControlManager();

				if (CCPCallImpl.this.callControlManager != null) {
					CCPCallImpl.this.mediaManager = CCPCallImpl.this.twBinder.getMediaManager();
					CCPCallImpl.this.audRecordManager = CCPCallImpl.this.twBinder.getAudioRecordManager();
					CCPCallImpl.this.mediaPlayManager = CCPCallImpl.this.twBinder.getMediaPlayManager();
					CCPCallImpl.this.callControlManager.postCommand(new Runnable() {

						@Override
						public void run() {
							// callback ui when initialized finished
							initListener.onInitialized();
						}
						
					});
				} else {
					Exception error = CCPCallImpl.this.twBinder.getError();
					initListener.onError(error);
				}
			}

			@Override
			public void onServiceDisconnected(ComponentName name) {
				sdkIniting = false;
				sdkInited = false;
				CCPCallImpl.this.callControlManager = null;
				CCPCallImpl.this.mediaManager = null;
				CCPCallImpl.this.twBinder = null;
				CCPCallImpl.this.context = null;
			}
		};

		if (!this.context.bindService(service, this.serviceConn, Context.BIND_AUTO_CREATE)) {
			this.context = null;
			initListener.onError(new RuntimeException("Failed to start CCPService. Please ensure it is declared in AndroidManifest.xml."));
		} 
	}

	void reverseInitialize(Context context, CCPService.CCPCallBinder twBinder, List<Device> devicesInfo) {
		this.sdkIniting = true;

		this.context = context;
		this.twBinder = twBinder;

		this.callControlManager = twBinder.getCallControlManager();
		if (this.callControlManager != null) {
			this.mediaManager = twBinder.getMediaManager();

			this.serviceConn = new ServiceConnection() {

				public void onServiceConnected(ComponentName name, IBinder binder) {

				}

				public void onServiceDisconnected(ComponentName name) {
					sdkIniting = false;
					sdkInited = false;
					
					CCPCallImpl.this.callControlManager = null;
					CCPCallImpl.this.mediaManager = null;
					CCPCallImpl.this.twBinder = null;
					CCPCallImpl.this.context = null;
				}
			};

			Intent intent = new Intent(context, CCPService.class);
			if (!context.bindService(intent, this.serviceConn, Context.BIND_AUTO_CREATE)) {
				Log4Util.e(Device.TAG, "Failed to re-initialize SDK: could not bind to service");
				this.serviceConn.onServiceDisconnected(new ComponentName(context, CCPService.class));
				this.serviceConn = null;
			} else {
				this.sdkIniting = false;
				this.sdkInited = true;
				
				synchronized (this.devices) { 
					for (Device deviceInfo :  devicesInfo) { 
						
						try {
							DeviceImpl device = new DeviceImpl((DeviceImpl)deviceInfo);
							this.devices.put(device.getUUID(), device); 
						} catch (Exception e) {
							e.printStackTrace();
						}
					} 
				}
			}
		} else {
			Exception error = twBinder.getError();
			Log4Util.e(Device.TAG, "Failed to re-initialize SDK: " + (error != null ? error.getLocalizedMessage() : "(unknown error)"));
			this.sdkIniting = false;
			this.sdkInited = false;
		}
	}
	
	public Device createDevice(DeviceListener deviceListener, Map<String, String> params) {
		if (!isInitialized()) {
			Log4Util.e(Device.TAG, "Device.create() called without a successful call to Device.init()");
			return null;
		}

		
		DeviceImpl device = null;
		try {
			device = new DeviceImpl(deviceListener, params);
			this.devices.put(device.getUUID(), device);
			updateServiceState();
		} catch (Exception e) {
			e.printStackTrace();
			throw new RuntimeException(e);
		}
		
		return device;
	}

	@SuppressWarnings("unchecked")
	public List<Device> listDevices() {
		synchronized (this.devices) {
			return Collections.unmodifiableList(new ArrayList(this.devices.values()));
		}
	}
	
	private void updateServiceState() {
		if ((this.context == null) || (this.twBinder == null)) {
			Log4Util.e(Device.TAG, "Device.updateServiceState() called but context or binder is null.");
			return;
		}
		
		Intent intent = new Intent(this.context, CCPService.class);
		synchronized (this.devices) {
			int nDevices = this.devices.size();
			if (nDevices > 0) {
				ArrayList<Device> devicesInfo = new ArrayList<Device>(nDevices);
				for (DeviceImpl device : this.devices.values()) {
					devicesInfo.add(device);
				}
				intent.setAction(CCPService.ACTION_DEVICES);
				intent.putParcelableArrayListExtra(Device.DEVICES_ARRAY, devicesInfo);
			}
		}
		this.context.startService(intent);
	}
	
	DeviceImpl findDeviceByUUID(UUID uuid) {
		synchronized (this.devices) {
			return (DeviceImpl) this.devices.get(uuid);
		}
	}

	void deviceChanged(DeviceImpl device) {
		updateServiceState();
	}
	
	public NetworkState getNetworkState() {
		return this.twBinder.getNetworkState();
	}
	
	/*public void setAlarmTime() {
		this.twBinder.setAlarmTime();
	}*/
	
	public APN getAPN() {
		return this.twBinder.getAPN();
	}
	
	@SuppressWarnings("unchecked")
	public void shutdown() {
		if (!isInitialized()) {
			if (isInitializing()) {
				Log4Util.w("CCPCallImpl", "Device.shutdown() called before Device.init() has finished");
			} else {
				Log4Util.e("CCPCallImpl", "Device.shutdown() called before Device.init()");
			}
			return;
		}

		try {
			if (this.devices != null) {
				for (Map.Entry entry : this.devices.entrySet()) {
					DeviceImpl device = (DeviceImpl) entry.getValue();
					if (device != null) {
						device.destroy();
						device = null;
					}
				}
				this.devices.clear();
			}

			this.callControlManager = null;
			this.mediaManager = null;
			this.twBinder = null;

			this.context.unbindService(this.serviceConn);
			this.context.stopService(new Intent(this.context, CCPService.class));
			this.serviceConn = null;
			this.context = null;
			this.sdkInited = false;
			this.sdkIniting = false;
			instance = null;
			Log4Util.w(Device.TAG, "Device.shutdown finished.");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	
	
	public void destroyDevice() {
		
	}
	
	/**
	 * @return the context
	 */
	public Context getContext() {
		return context;
	}

	/**
	 * @return the callControlManager
	 */
	public CallControlManager getCallControlManager() {
		return callControlManager;
	}

	/**
	 * @return the mediaManager
	 */
	public MediaManager getMediaManager() {
		return mediaManager;
	}
	
	/**
	 * @return the recordManager
	 * @deprecated
	 */
	public RecordManager getRecordManager() {
		return recordManager;
	}
	

	/**
	 * @return the recordManager
	 */
	public AudioRecordManager getAudioRecordManager() {
		return audRecordManager;
	}
	
	
	/**
	 * @return the mediaPlayManager
	 */
	public MediaPlayManager getMediaPlayManager() {
		return mediaPlayManager;
	}
}
