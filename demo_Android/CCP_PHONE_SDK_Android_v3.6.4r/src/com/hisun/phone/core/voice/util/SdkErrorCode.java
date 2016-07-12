package com.hisun.phone.core.voice.util;

/**
 * 
 * @author Jorstin Chan
 * @version 3.4.1.1
 */
public class SdkErrorCode {

	public static final int REQUEST_SUCCESS       = 0;
	/**
	 * The error code for rest that Need to hang up call.
	 */
	public static final int REST_INTERPHONE_NOTEXIST 			= 111609;
	public static final int REST_CHATROOM_NOTEXIST 				= 111703;
	public static final int REST_VIDEOCONFERENCE_NOTEXIST 		= 111805;
	
	public static final int SDK_NOT_REGISTED 		= 170000;
	public static final int SDK_CALL_BUSY	 		= 170001;
	public static final int SDK_XML_ERROR 			= 170002;
	public static final int SDK_STATUSCODE_ERROR 	= 170003;
	public static final int SDK_XMLBODY_ERROR 		= 170004;
	
	public static final int SDK_HTTP_ERROR 			= 170005;
	public static final int SDK_MAKECALL_FAILED 	= 170006;
	public static final int SDK_VERSION_NOTSUPPORT 	= 170007;
	public static final int SDK_JOIN_FAILED 		= 170008;
	public static final int SDK_UNKNOW_ERROR 		= 170009;
	public static final int SDK_NET_CONNETING 		= 170010;
	public static final int SDK_WRITE_FAILED		= 170011;
	
	
	/**
	 * The sender canceled the audio transmission
	 */
	public static final int SDK_AMR_CANCLE 			= 170016;
	
	/**
	 * The file server does not exist in the current need to download files
	 */
	public static final int SDK_FILE_NOTEXIST		= 170017;
	public static final int SDK_REQUEST_TIMEOUT    = 170019;
	/**
	 * limit of text
	 */
	public static final int SDK_TEXT_LENGTH_LIMIT	= 170022;
}
