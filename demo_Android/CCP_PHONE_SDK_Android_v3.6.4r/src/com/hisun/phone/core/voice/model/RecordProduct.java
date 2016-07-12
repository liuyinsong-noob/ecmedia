package com.hisun.phone.core.voice.model;

public class RecordProduct extends Response {
	/**
	 * 
	 */
	private static final long serialVersionUID = -4952973778049688030L;
	public RecordProduct(RecordProductType type)
	{
		productType = type;
	}
	public RecordProductType productType;
	public byte[] data;
	public RecourProductInfo info;
	
	public static enum RecordProductType
	{
		ProductStart,
		ProductInfo,
		ProductData,
		ProductEnd
	}
}
