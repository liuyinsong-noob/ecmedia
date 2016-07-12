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
package com.hisun.phone.core.voice.util;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.text.TextUtils;

import com.hisun.phone.core.voice.Device.RunningType;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;

/**
 * <p>Title: ConferenceUtils.java</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2014</p>
 * <p>Company: Beijing Speedtong Information Technology Co.,Ltd</p>
 * @author Jorstin Chan
 * @date 2014-5-7
 * @version 1.0
 */
public class ConferenceUtils {

	public static HashMap<String , String> mConferenceMap = new HashMap<String, String>();
	
	
	static Object mLock = new Object();
	public static void procesConferenceNo(RunningType type ,String confNo) {
		synchronized (mLock) {
			if(type == RunningType.RunningType_Interphone) {
				mConferenceMap.put(confNo, confNo);
				return;
			}
			
			if(type == RunningType.RunningType_ChatRoom
					|| type == RunningType.RunningType_VideoConference) {
				String procesConfNo = getConferenceNo(confNo);
				mConferenceMap.put(procesConfNo, confNo);
				return;
			}
		}
	}
	
	public static String getProcesConferenceNo(String originalConfNo) {
		synchronized (mLock) {
			for(Map.Entry<String, String> entry : mConferenceMap.entrySet()) {
				String value = entry.getValue();
				if(!TextUtils.isEmpty(value) && value.equals(originalConfNo)) {
					return entry.getKey();
				}
			}
			return originalConfNo;
		}
	}
	
	public static String getOriginalConferenceNo(String procesConfNo) {
		synchronized (mLock) {
			for(Map.Entry<String, String> entry : mConferenceMap.entrySet()) {
				String value = entry.getKey();
				if(!TextUtils.isEmpty(value) && value.equals(procesConfNo)) {
					return entry.getValue();
				}
			}
			return procesConfNo;
		}
	}
	
	public static void releaseConferMapCache() {
		if(!mConferenceMap.isEmpty()) {
			for(Map.Entry<String, String> entry : mConferenceMap.entrySet()) {
				mConferenceMap.remove(entry.getKey());
			}
			mConferenceMap.clear();
		}
	}
	
	
	public static List<Chatroom> procesChatrooms(List<Chatroom> chatrooms) {
		if(chatrooms != null && !chatrooms.isEmpty()) {
			for(Chatroom chatroom : chatrooms) {
				String roomNo = chatroom.getRoomNo();
				procesConferenceNo(RunningType.RunningType_ChatRoom, roomNo);
				chatroom.setRoomNo(getProcesConferenceNo(roomNo));
			}
		}
		return chatrooms;
	}

	public static List<VideoConference> procesVideoConferences(List<VideoConference> videoConferences) {
		if(videoConferences != null && !videoConferences.isEmpty()) {
			for(VideoConference videoConference : videoConferences) {
				String roomNo = videoConference.getConferenceId();
				procesConferenceNo(RunningType.RunningType_VideoConference, roomNo);
				videoConference.setConferenceId(getProcesConferenceNo(roomNo));
			}
		}
		return videoConferences;
	}
	
	public static  String getConferenceNo(String conferenceNo) {
		if(TextUtils.isEmpty(conferenceNo) || conferenceNo.length() <30) {
			return conferenceNo;
		}
		
		return conferenceNo.substring(0, 30);
	}
}
