/**
 * 
 */
package com.hisun.phone.core.voice.util;

import com.hisun.phone.core.voice.Device;

import android.os.Handler;
import android.os.Looper;

public final class CallCommandHandlerImpl extends Thread implements
		CallCommandHandler {

	private Looper looper;

	private Handler callHandler;

	CallCommandHandlerImpl() {
		start();
	}

	@Override
	public void destroy() {
		try {
			if (looper != null) {
				looper.quit();
			}
			callHandler = null;
			interrupt();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public void postCommand(Runnable command) {
		if (this.callHandler != null && currentThread().isAlive()) {
			this.callHandler.post(command);
			Log4Util
					.d(Device.TAG,
							"[CallCommandHandlerImpl - postCommand] post command finish.");
		}
	}

	public void postCommand(Runnable command, long delay) {
		if (this.callHandler != null && currentThread().isAlive()) {
			this.callHandler.postDelayed(command, delay);
			Log4Util
					.d(Device.TAG,
							"[CallCommandHandlerImpl - postCommand] post command finish.");
		}
	}

	public void run() {
		try {
			Thread.currentThread().setName("CallCommandHandlerImpl");
			Log4Util.d(Device.TAG,
					"[CallCommandHandlerImpl - run] thread already running: "
							+ Thread.currentThread().getName());
			Looper.prepare();
			this.looper = Looper.myLooper();
			this.callHandler = new Handler();
			Looper.loop();
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			destroy();
			Log4Util.d(Device.TAG,
					"CallCommandHandlerImpl thread was destroyed.");
		}
	}

	@Override
	public Handler getCommandHandler() {
		if (callHandler == null) {
			Log4Util.e(Device.TAG,"[CallCommandHandlerImpl - getCommandHandler] can't get command handler, it's null, recreate it?");
		}
		return callHandler;
	}
}
