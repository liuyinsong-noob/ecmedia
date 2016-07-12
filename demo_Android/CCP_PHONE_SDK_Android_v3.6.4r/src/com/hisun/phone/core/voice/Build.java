package com.hisun.phone.core.voice;

import com.hisun.phone.core.voice.util.VoiceUtil;

/**
 * 
 * @ClassName: Build.java
 * @author Jorstin Chan 
 * @date 2013-12-9
 * @version 3.6
 */
public class Build {
	
	public static final int SDK_INT = 100364;

	public static final String SDK_VERSION = "3.6.4";
	/**
	 * The SDK versoin code
	 */
	public static final String SDK_VERSION_NAME = "Android 3.6.4.1410301651";
	
	/**
	 * The CCP SDK rest server version code 
	 */
	@Deprecated static final String REST_VERSION_20130322 = "2013-03-22" ;
	
	/**
	 * The ccp sdk rest server version vode 1226
	 * 
	 * @version 3.6
	 */
	public static final String REST_VERSION = "2013-12-26" ;
	
	/**
	 * [version=1.1.23.6, 
	 * platform=Android, 
	 * ARMVersion=armv5, 
	 * audioSwitch=false, 
	 * videoSwitch=true, 
	 * compileDate=Mar  7 2014 15:01:36]
	 */
	public static class LIBVERSION {
		
		public static final String FULL_VERSION = VoiceUtil.getVersoinString("full");
		 /**
         * The user-visible version string.  E.g., "1.0" or "3.4b5".
         */
		public static final String RELEASE = VoiceUtil.getVersoinString("version");
		
		/**
		 * [version=1.1.23.6, platform=Android, ARMVersion=armv5, audioSwitch=false, videoSwitch=true, compileDate=Mar  7 2014 15:01:36]
		 */
		public static final String PLATFORM = VoiceUtil.getVersoinString("platform");
		
		/**
		 * 
		 */
		public static final String ARM_VERSION = VoiceUtil.getVersoinString("ARMVersion");
		
		/**
		 * 
		 */
		public static final boolean AUDIO_SWITCH = Boolean.valueOf(VoiceUtil.getVersoinString("audioSwitch")).booleanValue();
		
		/**
		 * 
		 */
		public static final boolean VIDEO_SWITCH = Boolean.valueOf(VoiceUtil.getVersoinString("videoSwitch")).booleanValue();
		
		/**
		 * 
		 */
		public static final String COMPILE_DATE = VoiceUtil.getVersoinString("compileDate");

		
	}
	
	
	public static String toVersionString() {
		return "LIBSERPHONE_VERSION [version=" + LIBVERSION.RELEASE 
		+ " ,platform=" + LIBVERSION.PLATFORM 
		+ " ,ARMVersion= " + LIBVERSION.ARM_VERSION 
		+ " ,audioSwitch=" + LIBVERSION.AUDIO_SWITCH 
		+ " ,videoSwitch=" + LIBVERSION.VIDEO_SWITCH 
		+ " ,compileDate=" + LIBVERSION.COMPILE_DATE  + "]";
	};
}
