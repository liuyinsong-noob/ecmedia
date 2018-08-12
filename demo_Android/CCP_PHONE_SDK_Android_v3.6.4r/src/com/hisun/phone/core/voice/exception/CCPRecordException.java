/**
 * 
 */
package com.hisun.phone.core.voice.exception;

/**
 * <p>Title: CCPRecordException</p>
 * <p>Description: This exception is thrown to Recording error. </p>
 * <p>Company: http://www.yuntongxun.com/</p>
 * @author  Jorstin Chan
 * @version 3.6
 * @date 2013-12-25
 */
public class CCPRecordException extends CCPException {

	
	/**
	 * 
	 */
	private static final long serialVersionUID = -6952784057211175759L;

	/**
     * Constructs a new {@code CCPRecordException} that includes the current stack
     * trace.
     */
    public CCPRecordException() {
    	
    }

    /**
     * Constructs a new {@code CCPRecordException} with the current stack trace
     * and the specified detail message.
     *
     * @param detailMessage
     *            the detail message for this exception.
     */
    public CCPRecordException(String detailMessage) {
        super(detailMessage);
    }
}
