/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.tools;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.media.MediaPlayer;
import android.os.Environment;
import android.os.Handler;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.WindowManager;

import com.CCP.phone.CameraCapbility;
import com.CCP.phone.VideoSnapshot;
import com.hisun.phone.core.voice.CCPCall;
import com.hisun.phone.core.voice.Device.AudioMode;
import com.hisun.phone.core.voice.Device.AudioType;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.UDPSocketUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.sqlite.CCPSqliteManager;
import com.voice.demo.ui.CCPHelper;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Random;

/**
 * Simple tools.
 * 
 * @version 1.0.0
 *
 */
public final class CCPUtil {

	public static final String DEMO_ROOT_STORE = "voiceDemo";
	
	private static float density = -1.0F;
	
	/**
	 * 是否有外存卡
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
	 * 通话时间 格式00:00:00
	 * 
	 * @param duration
	 * @return
	 */
	public static String getCallDurationShow(long duration) {
		if (duration / 60 == 0) {
			String second = "00";

			if (duration < 10) {
				second = "0" + duration;
			} else {
				second = duration + "";
			}
			return "00:" + second;
		} else {
			String minute = "00";
			String second = "00";
			String hour = "00";
			if ((duration / 60) < 10) {
				minute = "0" + (duration / 60);
			} else {
				if ((duration / 60) > 59) {
					if ((duration / 3600) < 10) {
						hour = "0" + (duration / 3600);
					} else {
						hour = (duration / 3600) + "";
					}

					if ((duration / 60) % 60 < 10) {
						minute = "0" + (duration / 60) % 60;
					} else {
						minute = (duration / 60) % 60 + "";
					}

				} else {
					minute = (duration / 60) + "";
				}
			}
			if ((duration % 60) < 10) {
				second = "0" + (duration % 60);
			} else {
				second = (duration % 60) + "";
			}
			if (hour.equals("00")) {
				return minute + ":" + second;
			} else {
				return hour + ":" + minute + ":" + second;
			}
		}
	}
	
	
	static MediaPlayer mediaPlayer = null;
	public static void playNotifycationMusic (Context context ,String voicePath) throws IOException  {
		//paly music ...
		AssetFileDescriptor fileDescriptor = context.getAssets().openFd(voicePath);
		if(mediaPlayer == null ) {
			mediaPlayer = new MediaPlayer();
		}
		if(mediaPlayer.isPlaying()) {
			mediaPlayer.stop();
		}
		mediaPlayer.reset();
		mediaPlayer.setDataSource(fileDescriptor.getFileDescriptor(),fileDescriptor.getStartOffset(),
				fileDescriptor.getLength());
		mediaPlayer.prepare();
		mediaPlayer.setLooping(false);
		mediaPlayer.start();
	}
	
	public static ArrayList<String> removeString(List<String> strArr,String str){
		ArrayList<String> newArr= null;
		if(strArr!=null&&str!=null){
			newArr = new ArrayList<String>();
			for (String string : strArr) {
				if(!str.equals(string)){
					newArr.add(string);
				}
			}
		}
		return newArr;
	}
	
	public static void registDialog (final Context context) {
		new AlertDialog.Builder(context)
		 .setTitle(R.string.str_dialog_title) 
		 .setMessage(R.string.str_regist_again)
		 	.setPositiveButton(R.string.dialog_btn, new OnClickListener() {
				
				@Override
				public void onClick(DialogInterface dialog, int which) {
					
					try {
						//VoiceApplication.getInstance().setQuitApplication(true);
						clearActivityTask(context);
					} catch (Exception e) {
						e.printStackTrace();
					}
					
				}

			})
		 	.show();
	}
	

	public static void exitCCPDemo() {
		CCPHelper.getInstance().release();
		CCPConfig.release();
		CCPSqliteManager.getInstance().destroy();
		
		CCPApplication.getInstance().putDemoAccounts(null);
		
		if(CCPApplication.interphoneIds != null) {
			CCPApplication.interphoneIds.clear();
		}
		
		if(CCPApplication.chatRoomIds != null) {
			CCPApplication.chatRoomIds.clear();
		}
		// just for demo
		CCPCall.shutdown();
	}
	
	public static void clearActivityTask(final Context context) {
		Intent i = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName()); 
		i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP); 
		context.startActivity(i);
		((Activity)context).finish();
		
	}
	
	
	public static String getDateCreate() {
		SimpleDateFormat dateformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		return dateformat.format(new Date());
	}
	
	public static AudioMode getAudioMode(AudioType type, int value)
	{
		switch (type)
		{
			case AUDIO_AGC:
			{
				if (value == 0)
					return AudioMode.kAgcUnchanged;
				else if (value == 1)
				{
					return AudioMode.kAgcDefault;
				}
				else if (value == 2)
				{
					return AudioMode.kAgcAdaptiveAnalog;
				}
				else if (value == 3)
				{
					return AudioMode.kAgcAdaptiveDigital;
				}
				else if (value == 4)
				{
					return AudioMode.kAgcFixedDigital;
				}
			}
				break;
			case AUDIO_EC:
			{
				if (value == 0)
					return AudioMode.kEcUnchanged;
				else if (value == 1)
				{
					return AudioMode.kEcDefault;
				}
				else if (value == 2)
				{
					return AudioMode.kEcConference;
				}
				else if (value == 3)
				{
					return AudioMode.kEcAec;
				}
				else if (value == 4)
				{
					return AudioMode.kEcAecm;
				}
			}
				break;
			case AUDIO_NS:
			{
				if (value == 0)
					return AudioMode.kNsUnchanged;
				else if (value == 1)
				{
					return AudioMode.kNsDefault;
				}
				else if (value == 2)
				{
					return AudioMode.kNsConference;
				}
				else if (value == 3)
				{
					return AudioMode.kNsLowSuppression;
				}
				else if (value == 4)
				{
					return AudioMode.kNsModerateSuppression;
				}
				else if (value == 5)
				{
					return AudioMode.kNsHighSuppression;
				}
				else if (value == 6)
				{
					return AudioMode.kNsVeryHighSuppression;
				}
			}
				break;
	
			default:
				break;
		}
		
		return AudioMode.kAgcDefault;
	}
	
	public static final SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

	public static final SimpleDateFormat sequenceFormat = new SimpleDateFormat("yyyyMMddHHmmss");
	public static final String HOLDPLACE = nextHexString(49);
	
	/** The random seed */
	static final long seed = System.currentTimeMillis();
	// static final long seed=0;

	public static int K = 1;
	public static String getSequenceFormat(long t) {
		//long t = System.currentTimeMillis();
		Date d = new Date(t);
		String date = sequenceFormat.format(d);
		
		return date + "$" + (K++) + "%" + HOLDPLACE + "@" + t;
	}
	
	/** Returns a random hexadecimal String */
	public static String nextHexString(int len) {
		byte[] buff = new byte[len];
		for (int i = 0; i < len; i++) {
			int n = nextInt(16);
			buff[i] = (byte) ((n < 10) ? 48 + n : 87 + n);
		}
		return new String(buff);
	}
	
	
	/** Returns a random integer between 0 and n-1 */
	public static int nextInt(int n) {
		Random rand = new Random(seed);
		return Math.abs(rand.nextInt()) % n;
	}
	
	
	public static void networkMonitor(final Context context , final Handler mHandler) {
		final UDPSocketUtil mSocketLayer = new UDPSocketUtil();
		mSocketLayer.setonSocketLayerListener(new UDPSocketUtil.onSocketLayerListener() {
			
			@Override
			public void onSocketLayerComplete() {
				int count = (mSocketLayer.getSendPacketCount() == 0 ? 0: (((mSocketLayer.getSendPacketCount()-mSocketLayer.getRecePacketCount()) * 100)/(mSocketLayer.getSendPacketCount())));
				if(count >= 30) {
					mHandler.sendEmptyMessage(1);
					
					}
				}
		});
		mSocketLayer.start();
		
	}
	
	// 锟叫讹拷锟街凤拷锟角凤拷锟斤拷锟斤拷锟�
	public static boolean hasFullSize(String inStr) {
		if (inStr.getBytes().length != inStr.length()) {
			return true;
		}
		return false;
	}
	
	public static final String TACK_PIC_PATH = getExternalStorePath()+  "/" +DEMO_ROOT_STORE + "/.chatTemp";
	public static File TackPicFilePath() {
		File localFile = new File(TACK_PIC_PATH , createCCPFileName() + ".jpg");
		if ((!localFile.getParentFile().exists())
				&& (!localFile.getParentFile().mkdirs())) {
			localFile = null;
		}
		return localFile;
	}
	public static final String TACK_VIDEO_PATH = getExternalStorePath()+  "/" +DEMO_ROOT_STORE + "/.videoTemp";
	public static File TackVideoFilePath() {
		File localFile = new File(CCPUtil.TACK_VIDEO_PATH , CCPUtil.createCCPFileName() + ".mp4");
		if ((!localFile.getParentFile().exists())
				&& (!localFile.getParentFile().mkdirs())) {
			localFile = null;
		}
		return localFile;
	}
	
	/**
	 * /sdcard
	 *
	 * @return
	 */
	public static String getExternalStorePath() {
		if (isExistExternalStore()) {
			return Environment.getExternalStorageDirectory().getAbsolutePath();
		}
		return null;
	}
	
	
	/**
	 * dip转化像素
	 * @param context
	 * @param dipValue
	 * @return
	 */
	public static int dip2px(Context context, float dipValue){

		final float scale = context.getResources().getDisplayMetrics().density;

		return (int)(dipValue * scale + 0.5f);

	}
	
	/**
	 * @param context
	 * @return
	 */
	public static float getDensity(Context context) {
		if (context == null)
			context = CCPApplication.getInstance().getApplicationContext();
		if (density < 0.0F)
			density = context.getResources().getDisplayMetrics().density;
		return density;
	}
	
	public static int round(Context context, int paramInt) {
		return Math.round(paramInt / getDensity(context));
	}
	
	public static int fromDPToPix(Context context, int dp) {
		return Math.round(getDensity(context) * dp);
	}

	
	
	private static long lastClickTime; 
    public static boolean isInvalidClick() { 
        long time = System.currentTimeMillis(); 
        long timeD = time - lastClickTime; 
        if ( 0 < timeD && timeD < 1000) {    
            return true;    
        }    
        lastClickTime = time;    
        return false;    
    } 
    
    
    public static boolean delFile(String filePath){
    	File file = new File(filePath);
		if (!file.exists()) {
			return true;
		}
		
		return file.delete();
    }

	public static void delFolder(String folderPath) {
		try {
			delAllFile(folderPath); // 删除完里面所有内容
			String filePath = folderPath;
			filePath = filePath.toString();
			File myFilePath = new File(filePath);
			myFilePath.delete(); // 删除空文件夹
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * delete all file
	 * @param path
	 * @return
	 */
	public static boolean delAllFile(String path) {
		boolean flag = false;
		File file = new File(path);
		if (!file.exists()) {
			return flag;
		}
		if (!file.isDirectory()) {
			return flag;
		}
		String[] tempList = file.list();
        if(tempList == null)
            return flag;
		File temp = null;
		for (int i = 0; i < tempList.length; i++) {
			if (path.endsWith(File.separator)) {
				temp = new File(path + tempList[i]);
			} else {
				temp = new File(path + File.separator + tempList[i]);
			}
			if (temp.isFile()) {
				temp.delete();
			}
			if (temp.isDirectory()) {
				delAllFile(path + "/" + tempList[i]);// 先删除文件夹里面的文件
				delFolder(path + "/" + tempList[i]);// 再删除空文件夹
				flag = true;
			}
		}
		return flag;
	}

	public static int getMetricsDensity(Context context , float height) {
		DisplayMetrics localDisplayMetrics = new DisplayMetrics();
		((WindowManager) context.getSystemService(Context.WINDOW_SERVICE))
				.getDefaultDisplay().getMetrics(localDisplayMetrics);
		return Math.round(height * localDisplayMetrics.densityDpi / 160.0F);
	}
	
	public static SDKVersion getSDKVersion(String version) {
		// 1.1.18#Android#armv7#voice=true#video=true#Sep 12 2013 13:41:26
		try {
			if(TextUtils.isEmpty(version)) {
				return null;
			}
			SDKVersion sdkVersion = new SDKVersion();
			String[] split = version.split("#");
			sdkVersion.setVersion(split[0]);
			sdkVersion.setPlatform(split[1]);
			sdkVersion.setARMVersion(split[2]);
			if(split[3].startsWith("audio")) {
				String[] voice = split[3].split("=");
				sdkVersion.setAudioSwitch(Boolean.valueOf(voice[1]).booleanValue());
			}
			if (split[4].startsWith("video")) {
				String[] video = split[4].split("=");
				sdkVersion.setVideoSwitch(Boolean.valueOf(video[1]).booleanValue());
			}
			sdkVersion.setCompileDate(split[split.length - 1]);
			return sdkVersion;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return null;
	}
	
	
	public static String createCCPFileName() {
		Calendar c = Calendar.getInstance();
		c.setTimeInMillis(System.currentTimeMillis());
		
		int y = c.get(Calendar.YEAR);
		int m = c.get(Calendar.MONTH);
		int d = c.get(Calendar.DAY_OF_MONTH);
		int hour = c.get(Calendar.HOUR_OF_DAY);
		int minute = c.get(Calendar.MINUTE);
		int second = c.get(Calendar.SECOND);
		return y + "-" + m  + "-" + d  + "-" + hour + "-" + minute  + "-" + second;
	}
	
	
	
	public static String interceptStringOfIndex(String str , int index) {
		String intercept = str;
		
		if(TextUtils.isEmpty(str)) {
			return str;
		}
		
		if(str.length() > index) {
			intercept = str.substring(str.length() - index , str.length());
			
		}
		
		return intercept;
	}
	
	/**
	 * 将文件生成位图
	 * 
	 * @param path
	 * @return
	 * @throws IOException
	 */
	public static BitmapDrawable getImageDrawable(String path) throws IOException {
		// 打开文件
		File file = new File(path);
		if (!file.exists()) {
			return null;
		}

		ByteArrayOutputStream outStream = new ByteArrayOutputStream();
		byte[] bt = new byte[1024];

		// 得到文件的输入流
		InputStream in = new FileInputStream(file);

		// 将文件读出到输出流中
		int readLength = in.read(bt);
		while (readLength != -1) {
			outStream.write(bt, 0, readLength);
			readLength = in.read(bt);
		}

		// 转换成byte 后 再格式化成位图
		byte[] data = outStream.toByteArray();
		Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);// 生成位图
		BitmapDrawable bd = new BitmapDrawable(bitmap);

		return bd;
	}
	
	public static final int RANDOM_STRING_NONE = 0;
	public static final int RANDOM_STRING_CHAR = 1;
	public static final int RANDOM_STRING_NUM = 2;
	// Get 6 numbers and the composition string.
	public static String getCharAndNumr(int length ,int type) {
		String val = "";
		Random random = new Random();
		for (int i = 0; i < length; i++) {
			String charOrNum = random.nextInt(2) % 2 == 0 ? "char" : "num"; // 输出字母还是数字
			if(type == RANDOM_STRING_CHAR) {
				charOrNum = "char" ;
			} else if (type == RANDOM_STRING_NUM) {
				charOrNum = "num";
			}

			if ("char".equalsIgnoreCase(charOrNum)) // 字符串
			{
				int choice = /*random.nextInt(2) % 2 == 0 ? 65 : 97*/97; // 取得大写字母还是小写字母
				val += (char) (choice + random.nextInt(26));
			} else if ("num".equalsIgnoreCase(charOrNum)) // 数字
			{
				val += String.valueOf(random.nextInt(10));
			}
		}

		return val;
	}
	
	/**
	 * 
	* <p>Title: saveByteToFile</p>
	* <p>Description: 
	* byte[] data = lPortrait.getBytes() is RGB24 data</p>
	* @param lPortrait
	* @param savepath
	 */
	public static void saveByteToFile(VideoSnapshot lPortrait ,String savepath) {
		Log4Util.d(CCPHelper.DEMO_TAG , "LocalPortrait saveByteToFile savepath :" + savepath);
		if (lPortrait == null || savepath == null){
			
			return;
		}
		try {
			if (savepath.length() > 0) {
				
				/*YuvImage yuvimage=new YuvImage(lPortrait.getBytes(), ImageFormat.NV21, lPortrait.getWidth(),lPortrait.getHeight(), null);
				ByteArrayOutputStream baos = new ByteArrayOutputStream();
				yuvimage.compressToJpeg(new Rect(0, 0, lPortrait.getWidth(), lPortrait.getHeight()), 100, baos);// 80--JPG图片锟斤拷锟斤拷锟斤拷[0-100],100锟斤拷锟�
				byte[] jdata = baos.toByteArray();*/
				
				Bitmap bitmap = CCPBitmapUtils.decodeFrameToBitmap(lPortrait.data, lPortrait.width, lPortrait.height);
				
				if(bitmap == null ) {
					return;
				}
				//Bitmap bitmap = BitmapFactory.decodeByteArray(jdata, 0, jdata.length);
				File img = new File(savepath);
				FileOutputStream fos = null;
				try {
					fos = new FileOutputStream(img);
					bitmap.compress(CompressFormat.PNG, 100, fos);
				} catch (IOException e) {
					e.printStackTrace();
					Log4Util.v(e.getMessage());
				} finally {
					if (fos != null) {
						fos.flush();
						fos.close();
					}
					if (bitmap != null){
						bitmap.recycle();
						bitmap = null;
					}
				}
			} 
			
		} catch (Exception ex){
			ex.printStackTrace();
			Log4Util.e(ex.toString());
			}

	}
	
	public static String CALLS_RECORD_TEMP_PATH = CCPUtil.getExternalStorePath()+  "/" +CCPUtil.DEMO_ROOT_STORE + "/callsRecordTemp";
	
	/**
	 * @param fileName
	 * @param ext
	 * @return
	 */
	public static File createCallRecordFilePath(String fileName , String ext) {
		File localFile = new File(CALLS_RECORD_TEMP_PATH , fileName + "." + ext);
		if ((!localFile.getParentFile().exists())
				&& (!localFile.getParentFile().mkdirs())) {
			localFile = null;
		}
		
		return localFile;
	}
	
	
	/**
	 * @param caps
	 */
	public static  int comportCapbilityIndex(CameraCapbility[] caps) {
		if(caps == null ) {
			return 0;
		}
		int pixel[] = new int[caps.length];
		for(CameraCapbility cap : caps) {
			if(cap.index >= pixel.length) {
				continue;
			}
			pixel[cap.index] = cap.width * cap.height;
		}
		
		//System.arraycopy(pixel, 0, _pixel, 0, caps.length);
		
		//Arrays.sort(_pixel);
		for(int i = 0 ; i < caps.length ; i++) {
			if(pixel[i] == /*_pixel[0]*/ 640*480) {
				return i;
			}
		}
		
		return caps.length - 1;
	}
	
	public static String remove86(String phone) {
		if(TextUtils.isEmpty(phone)){
			return phone;
		}
		String str=phone;
		if(phone.startsWith("86"))
			str=phone.substring(2);
		else if(phone.startsWith("+86"))
			str=phone.substring(3);
		return str;
	}
	
}
