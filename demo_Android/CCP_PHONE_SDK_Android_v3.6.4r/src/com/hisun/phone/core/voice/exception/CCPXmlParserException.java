package com.hisun.phone.core.voice.exception;

/**
 * 
 * @ClassName: CCPXmlParserException.java
 * @Description: This exception is thrown to signal XML Pull Parser related faults.
 * @author Jorstin Chan 
 * @date 2013-12-19
 * @version 3.6
 */
public class CCPXmlParserException extends CCPException {

	/**
	 * 
	 */
	private static final long serialVersionUID = -833621989166873535L;
	
	/**
     * Constructs a new {@code CCPXmlParserException} that includes the current stack
     * trace.
     */
    public CCPXmlParserException() {
    	
    }

    /**
     * Constructs a new {@code CCPXmlParserException} with the current stack trace
     * and the specified detail message.
     *
     * @param detailMessage
     *            the detail message for this exception.
     */
    public CCPXmlParserException(String detailMessage) {
        super(detailMessage);
    }
}
