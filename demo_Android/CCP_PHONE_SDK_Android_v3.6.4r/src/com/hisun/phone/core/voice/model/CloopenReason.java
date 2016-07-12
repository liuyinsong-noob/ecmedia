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
 */package com.hisun.phone.core.voice.model;

import com.hisun.phone.core.voice.util.SdkErrorCode;

/**
 * 
 * <p>平台错误码</p>
 * @author yuntongxun.com 
 * @date 2014-1-6
 * @version 3.6
 */
public class CloopenReason extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -2287961413323658470L;

	/**
	 * 返回平台错误码Code
	 * @return
	 */
	public int getReasonCode() {
		
		try {
			Integer code = Integer.valueOf(this.statusCode);
			return code;
		} catch (Exception e) {
			return SdkErrorCode.SDK_STATUSCODE_ERROR;
		}
	}
	
	/**
	 * 返回平台错误码对应的错误描述
	 * @return
	 */
	public String getMessage() {
		return statusMsg;
	}
	
	public CloopenReason (){

	}
	
	public CloopenReason (int reason){
		this(reason, "");
	}
	
	public CloopenReason (int reason , String message){
		statusCode = reason + "";
		statusMsg = message;
	}
}
