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
 */package com.voice.demo.tools.net;

import com.voice.demo.videoconference.VideoconferenceBaseActivity;

/**
 * 
 * 
 * @ClassName: TaskKey.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-12
 * @version 3.6
 */
public class TaskKey {

	/**
	 * 
	 */
	public static final int TASK_KEY_REQUEST_FAILED = -1;
	/**
	 * query IM message undownload.
	 */
	public static final int KEY_QUERY_UNDOWNLOAD_IMMESSAGE = 0X1;
	
	/**
	 * CCP SDK regist.
	 */
	public static final int KEY_SDK_REGIST = 0x2;
	
	/**
	 * SDK unregist.
	 */
	public static final int KEY_SDK_UNREGIST = 0x3;
	
	/**
	 * Delete IM message .
	 */
	public static final int TASK_KEY_DEL_MESSAGE = 0x4;
	
	/**
	 * build test number.
	 */
	public static final int TASK_KEY_BUILD_NUMNBER = 0X5;
	
	public static final int TASK_KEY_AUTO_LOGOIN = 0X6;
	
	/**
	 * 
	 */
	public static final int TASK_KEY_LOADING_IM = 0X7;
	
	/**
	 * 
	 */
	public static final int TASK_KEY_VIDEO_FRAME = VideoconferenceBaseActivity.KEY_DELIVER_VIDEO_FRAME;
}
