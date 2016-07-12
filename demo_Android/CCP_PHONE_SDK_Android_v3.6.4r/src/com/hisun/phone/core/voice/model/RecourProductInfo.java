package com.hisun.phone.core.voice.model;

public class RecourProductInfo extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 5951188831440290869L;
	
	public String fileName;
	public String userData;
	public String groupId;
	
	// Note: 3.0.1 Add the parameters, 
	// namely the unique identifier for the audio message
	public String uniqueID;
}
