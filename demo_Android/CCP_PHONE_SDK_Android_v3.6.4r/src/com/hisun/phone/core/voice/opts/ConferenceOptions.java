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
package com.hisun.phone.core.voice.opts;

import java.io.Serializable;

/**
 * 创建一个视频会议参数选项，并调用视频会议接口创建视频会议
 * @author yuntongxun.com
 * @date 2014-8-26
 * @version 3.6.4
 */
public class ConferenceOptions implements Serializable{
	
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	/**
	 * 全部提示音
	 */
	public static final int DEFAULT_ALL = 1;
	
	/**
	 * 没有提示音有背景音
	 */
	public static final int DEFAULT_ONLY_COLORING_BACKGROUND_VOICE  = 0;
	
	/**
	 * 无提示音无背景音
	 */
	public static final int DEFAULT_NONE = 2;
	
	public ConferenceOptions() {
		inSquare = 5;
		inAutoClose = true;
		inVoiceMod = DEFAULT_ALL;
	}

	/**
	 * 会议方数 默认8方
	 */
	public int inSquare;
	
	/**
	 * 业务属性，由应用定义
	 */
	public String inKeywords;
	
	/**
	 * 创建者离开是否自动解散房间，
	 * 默认值为true创建者退出自动解散，false创建者退出房间不自动解散；
	 */
	public boolean inAutoClose;
	
	/**
	 * 是否自动删除(即是否永久会议)
	 * false永久( isAutoClose为false时有效)，true自动定时删除，
	 * 默认值为true自动删除。
	 */
	public boolean inAutoDelete;
	
	/**
	 * 0没有提示音有背景音、1全部提示音、2无提示音无背景音。默认值为1全部提示音
	 * {@link #DEFAULT_ALL}
	 * {@link #DEFAULT_NONE}
	 * {@link #DEFAULT_ONLY_COLORING_BACKGROUND_VOICE}
	 */
	public int inVoiceMod;
	
	/**
	 * false单路视频，true多路视频
	 */
	public boolean inMultiVideo;
	
	/**
	 * 是否是主持人模式  
	 * false不是  ,true是
	 */
	public boolean inPresenter;
}
