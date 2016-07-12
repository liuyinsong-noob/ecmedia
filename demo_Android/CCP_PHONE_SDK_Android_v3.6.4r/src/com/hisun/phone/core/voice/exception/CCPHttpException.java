package com.hisun.phone.core.voice.exception;

/**
 * 
 * @ClassName: CCPHttpException.java
 * @Description: TODO
 * @author Jorstin Chan
 * @date 2013-12-19
 * @version 3.6
 */
public class CCPHttpException extends CCPException {

	private static final long serialVersionUID = 1L;

	/**
	 * The server return an error status code when HTTP request failed, 
	 */
	private final int mStatusCode;

	/**
	 * Constructor method.
	 * 
	 * @param message
	 *            the detail message for this exception.
	 * @param statusCode
	 *            The server return an error status code when HTTP request failed, 
	 */
	public CCPHttpException(String message, int statusCode) {
		super(message);
		mStatusCode = statusCode;
	}

	/**
	 * The server return an error status code when HTTP request failed, 
	 * @return error code 
	 */
	public int getStatusCode() {
		return mStatusCode;
	}

}
