/**
 * 
 */
package com.hisun.phone.core.voice.util;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.util.Log;


public class Log4Util {
	private static boolean isPrint = true;
	
	public static final String TAG = "SDK_DEVICE";
	public static final String MSG = "log msg is null.";
	
	public static void init(boolean on) {
		isPrint = on;
		Log4Util.setPrint(false);
	}
	
	/**
	 * @return the isPrint
	 */
	public static boolean isPrint() {
		return isPrint;
	}


	public static void v(String tag, String msg) {
		print(Log.VERBOSE, tag, msg);
	}
	
	public static void v(String msg) {
		v(TAG, msg);
	}
	
	public static void d(String tag, String msg) {
		print(Log.DEBUG, tag, msg);
	}
	
	public static void d(String msg) {
		d(TAG, msg);
	}

	public static void i(String tag, String msg) {
		print(Log.INFO, tag, msg);
	}
	
	public static void i(String msg) {
		i(TAG, msg);
	}

	public static void w(String tag, String msg) {
		print(Log.WARN, tag, msg);
	}
	
	public static void w(String msg) {
		w(TAG, msg);
	}

	public static void e(String tag, String msg) {
		print(Log.ERROR, tag, msg);
	}
	
	public static void e(String msg) {
		e(TAG, msg);
	}

	private static void print(int mode, final String tag, String msg) {
		if (!isPrint) {
			return;
		}
		if (msg == null) {
			Log.e(tag, MSG);
			return;
		}
		switch (mode) {
		case Log.VERBOSE:
			Log.v(tag, msg);
			break;
		case Log.DEBUG:
			Log.d(tag, msg);
			break;
		case Log.INFO:
			Log.i(tag, msg);
			break;
		case Log.WARN:
			Log.w(tag, msg);
			break;
		case Log.ERROR:
			Log.e(tag, msg);
			break;
		default:
			Log.d(tag, msg);
			break;
		}
		
		/*if(!TextUtils.isEmpty(VoiceUtil.getExternalStorePath())) {
			String property = VoiceUtil.getPropertyLine();
			printFile(VoiceUtil.string2InputStream(msg + property), VoiceUtil.getExternalStorePath() + File.separator + TAG + ".log");
		}*/
	}
	
	public static void printFile(InputStream is, String path) {
		FileOutputStream fos = null;
		byte[] temp = null;
		try {
			if (isPrint) {
				File file = new File(path);
				if (!file.exists()) {
					file.createNewFile();
				}
				fos = new FileOutputStream(file , true);
				temp = new byte[1024];
				int i = 0;
				while ((i = is.read(temp)) > -1) {
					if (i < temp.length) {
						byte[] b = new byte[i];
						System.arraycopy(temp, 0, b, 0, b.length);
						fos.write(b);
					} else {
						fos.write(temp);
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (is != null) {
				try {
					is.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
				is = null;
			}
			if (fos != null) {
				try {
					fos.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
				fos = null;
			}
			temp = null;
		}
	}

	/**
	 * @param isPrint the isPrint to set
	 */
	public static void setPrint(boolean isPrint) {
		Log4Util.isPrint = isPrint;
	}
	
}
