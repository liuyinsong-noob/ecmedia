package com.hisun.phone.core.voice.util;

import android.os.Handler;

public abstract interface CallCommandHandler {
	public abstract void postCommand(Runnable paramRunnable);
	public abstract void destroy();
	public abstract Handler getCommandHandler();
}
