/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.group.model;

import com.hisun.phone.core.voice.model.Response;

public class IMGroup extends Response {
	
	public static final int MODEL_GROUP_PUBLIC = 0x0;
	public static final int MODEL_GROUP_AUTHENTICATION = 0x1;
	public static final int MODEL_GROUP_PRIVATE = 0x2;

	/**
	 * 
	 */
	private static final long serialVersionUID = 1317673848969906965L;

	public String groupId = null;//群组ID
	public String name = null;//群组名字
	public String type = null;//群组类型 0：临时组 1：普通组 2：VIP组
	public String declared = null;//群组公告
	public String createdDate = null; //该群组的创建时间
	public String permission = null;//申请加入模式 0：默认直接加入1：需要身份验证z
	
	public String owner;
	public String count;
	
}
