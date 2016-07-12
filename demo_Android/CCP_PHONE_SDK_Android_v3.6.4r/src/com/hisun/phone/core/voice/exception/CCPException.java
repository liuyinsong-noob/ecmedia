/**
 * 
 * @ClassName: CCPException.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-19
 * @version 3.6
 */
package com.hisun.phone.core.voice.exception;

/**
 * 
 * @ClassName: CCPException.java
 * @Description: TODO
 * @author Jorstin Chan
 * @date 2013-12-19
 * @version 3.6
 */
public class CCPException extends Exception {

	/**
	 * 
	 */
	private static final long serialVersionUID = 5493604650929251285L;

	/**
     * Constructs a new {@code CCPException} that includes the current stack
     * trace.
     */
    public CCPException() {
    }

    /**
     * Constructs a new {@code CCPException} with the current stack trace
     * and the specified detail message.
     *
     * @param detailMessage
     *            the detail message for this exception.
     */
    public CCPException(String detailMessage) {
        super(detailMessage);
    }

   /**
     * Constructs a new {@code CCPException} with the current stack trace,
     * the specified detail message and the specified cause.
     *
     * @param detailMessage
     *            the detail message for this exception.
     * @param throwable
     *            the cause of this exception.
     */
    public CCPException(String detailMessage, Throwable throwable) {
        super(detailMessage, throwable);
    }

    /**
     * Constructs a new {@code CCPException} with the current stack trace
     * and the specified cause.
     *
     * @param throwable
     *            the cause of this exception.
     */
    public CCPException(Throwable throwable) {
        super(throwable);
    }

}
