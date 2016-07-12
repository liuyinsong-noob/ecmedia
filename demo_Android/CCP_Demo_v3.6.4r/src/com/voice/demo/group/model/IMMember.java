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
package com.voice.demo.group.model;

import com.hisun.phone.core.voice.model.Response;

public class IMMember extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 604352159818994119L;

	public String voipAccount = null;//用户voip账号
	public String displayName = null;//用户名字
	public String sex = null;//性别
	public String birth = null;//用户生日
	public String tel = null;//用户电话
	public String sign = null;//用户的签名
	public String mail = null;//用户邮箱
	public String remark = null;//用户的备注
	public String belong = null;//所属群组
	
	public int isBan = 0;        // 是否禁言
	public int rule = 0;        // 是否接收群组消息
}
