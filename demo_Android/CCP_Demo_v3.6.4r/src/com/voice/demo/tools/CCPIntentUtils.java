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
package com.voice.demo.tools;

/**
 * @ClassName: CCPIntentUtils.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-12
 * @version 3.6
 */
public class CCPIntentUtils {

	/**
	 * Activity Action: Start a VoIP incomeing call.
	 */
	public static final String ACTION_VOIP_INCALL = "com.voice.demo.ACTION_VOIP_INCALL";

	/**
	 * Activity Action: Start a VoIP outgoing call.
	 */
	public static final String ACTION_VOIP_OUTCALL = "com.voice.demo.ACTION_VOIP_OUTCALL";

	/**
	 * Activity Action: Start a Video incomeing call.
	 */
	public static final String ACTION_VIDEO_INTCALL = "com.voice.demo.ACTION_VIDEO_INTCALL";

	/**
	 * Activity Action: Start a Video outgoing call.
	 */
	public static final String ACTION_VIDEO_OUTCALL = "com.voice.demo.ACTION_VIDEO_OUTCALL";

	// --------------------------------EXPERT -----------------------------------------
	/**
	 * Activity Action: Show expert main activity 
	 */
	public static final String ACTION_EXPERT_MAIN = "com.voice.demo.ACTION_EXPERT_MAIN";
	
	/**
	 * Activity Action: Show expert list activity
	 */
	public static final String ACTION_EXPERT_LIST_VIEW = "com.voice.demo.ACTION_EXPERT_LIST_VIEW";
	
	/**
	 * Activity Action: Show expert order view
	 */
	public static final String ACTION_EXPERT_ORDER = "com.voice.demo.ACTION_EXPERT_ORDER";
	
	/**
	 * Activity Action: Show expert communion view
	 */
	public static final String ACTION_EXPERT_CONMUI = "com.voice.demo.ACTION_EXPERT_CONMUI";
	
	/**
	 * Activity Action: Show expert detail view
	 */
	public static final String ACTION_EXPERT_DETAIL_VIEW = "com.voice.demo.ACTION_EXPERT_DETAIL_VIEW";
	
	// -------------------------------------IM Message -------------------------------------------------
	
	/**
	 * A broadcast intent that is sent when p2p was enable.
	 */
	public static final String INTENT_P2P_ENABLED = "com.voice.demo.INTENT_P2P_ENABLED";
	
	/**
	 * A broadcast intent that is sent when im message be deleted.
	 */
	public static final String INTENT_DELETE_GROUP_MESSAGE = "com.voice.demo.INTENT_DELETE_GROUP_MESSAGE";
	
	/**
	 * A broadcast intent that is sent when init private join group list total statistic
	 */
	public static final String INTENT_INIT_PRIVATE_GROUP_LIST = "com.voice.demo.INTENT_INIT_PRIVATE_GROUP_LIST";
	
	/**
	 * A broadcast intent that is sent when init public group list total statistic
	 */
	public static final String INTENT_JOIN_GROUP_SUCCESS = "com.voice.demo.INTENT_JOIN_GROUP_SUCCESS";
	
	/**
	 * A broadcast intent that is sent when remove member form group
	 */
	public static final String INTENT_REMOVE_FROM_GROUP = "com.voice.demo.INTENT_REMOVE_FROM_GROUP";
	
	/**
	 * A broadcast intent that is sent when dismiss group
	 */
	public static final String INTENT_DISMISS_GROUP = "com.voice.demo.INTENT_DISMISS_GROUP";
	
	/**
	 * A broadcast intent that is sent when receiver new system message
	 */
	public static final String INTENT_RECEIVE_SYSTEM_MESSAGE = "com.voice.demo.INTENT_RECEIVE_SYSTEM_MESSAGE";
	
	/**
	 * A broadcast intent that is sent when receiver new interphone chat
	 */
	public static final String INTENT_RECIVE_INTER_PHONE = "com.voice.demo.INTENT_RECIVE_INTER_PHONE";
	
	/**
	 * A broadcast intent that is sent when receiver new chatroom 
	 */
	public static final String INTENT_RECIVE_CHAT_ROOM = "com.voice.demo.INTENT_RECIVE_CHAT_ROOM";
	
	/**
	 * A broadcast intent that is sent when chatroom be dismiss
	 */
	public static final String INTENT_CHAT_ROOM_DISMISS = "com.voice.demo.INTENT_CHAT_ROOM_DISMISS";
	
	/**
	 * A broadcast intent that is sent when receiver new im message
	 */
	public static final String INTENT_IM_RECIVE = "com.voice.demo.INTENT_IM_RECIVE";
	
	/**
	 * A broadcast intent that is sent when account remote login
	 */
	public static final String INTENT_KICKEDOFF = "com.voice.demo.INTENT_KICKEDOFF";
	
	/**
	 * A broadcast intent that is sent when account remote login
	 */
	public static final String INTENT_INVALIDPROXY = "com.voice.demo.INTENT_INVALIDPROXY";
	
	/**
	 * A broadcast intent that is sent when Clients connect to the server
	 */
	public static final String INTENT_CONNECT_CCP = "com.voice.demo.INTENT_CONNECT_CCP";
	
	/**
	 * A broadcast intent that is sent when The client disconnect from the server
	 */
	public static final String INTENT_DISCONNECT_CCP = "com.voice.demo.INTENT_DISCONNECT_CCP";
	
	/**
	 * A broadcast intent that is sent when The voice message playback finishes
	 */
	public static final String INTENT_VOICE_PALY_COMPLETE = "com.voice.demo.INTENT_VOICE_PALY_COMPLETE";
	
	/**
	 * When received the dismiss action of video conference 
	 * Then refresh video conference list
	 */
	public static final String INTENT_VIDEO_CONFERENCE_DISMISS  = "com.voice.demo.INTENT_VIDEO_CONFERENCE_DISMISS";
}
