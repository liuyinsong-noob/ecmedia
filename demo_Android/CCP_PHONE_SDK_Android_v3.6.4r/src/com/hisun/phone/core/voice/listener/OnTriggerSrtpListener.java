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
package com.hisun.phone.core.voice.listener;

import com.hisun.phone.core.voice.Device;


/**
 * <p>呼叫时候底层主动触发该事件，在这里面设置加密，来电的时候同理</p>
 * @author yuntongxun.com
 * @date 2014-7-25
 * @version 1.0
 */
public interface OnTriggerSrtpListener {

	/**
	 * 加密接口 {@link Device#setSrtpEnabled(String)}
	 * @param callId
	 * @param caller true or false 
	 */
	public abstract void OnTriggerSrtp(String callId , boolean caller);
}
