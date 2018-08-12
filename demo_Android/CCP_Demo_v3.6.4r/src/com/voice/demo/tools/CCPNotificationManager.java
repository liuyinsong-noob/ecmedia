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
 */package com.voice.demo.tools;

import java.io.IOException;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import com.hisun.phone.core.voice.Device.CallType;
import com.voice.demo.R;
import com.voice.demo.group.GroupBaseActivity;
import com.voice.demo.group.GroupChatActivity;
import com.voice.demo.group.model.IMChatMessageDetail;
import com.voice.demo.interphone.InterPhoneRoomActivity;

/**
 * 
* <p>Title: CCPNotificationManager.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2007</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jor
* @date 2013-11-12
* @version 3.5
* 
 */
@SuppressWarnings("deprecation")
public class CCPNotificationManager {
	
	public static final int CCP_NOTIFICATOIN_ID_CALLING = 0x1;
	
	private static NotificationManager mNotificationManager;

	public static void showInCallingNotication(Context context ,CallType callType ,String topic, String text) {
		
		try {
			
			checkNotification(context);
			
			Notification notification = new Notification(R.drawable.icon_call_small, text,
					System.currentTimeMillis());
			notification.flags = Notification.FLAG_AUTO_CANCEL;
			notification.tickerText = topic;
			Intent intent = null;
			if(callType == CallType.VIDEO) {
				intent = new Intent(CCPIntentUtils.ACTION_VIDEO_INTCALL);
			} else {
				intent = new Intent(CCPIntentUtils.ACTION_VOIP_INCALL);
			}
			intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_NEW_TASK);  
			
			PendingIntent contentIntent = PendingIntent.getActivity(context, 
					R.string.app_name, 
					intent, 
					PendingIntent.FLAG_UPDATE_CURRENT);
			
			notification.setLatestEventInfo(context, 
					topic, 
					text, 
					contentIntent);
			
			mNotificationManager.notify(CCP_NOTIFICATOIN_ID_CALLING, notification);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public static void showOutCallingNotication(Context context ,CallType callType ,String topic, String text) {
		
		try {
			
			checkNotification(context);
			
			Notification notification = new Notification(R.drawable.icon_call_small, text,
					System.currentTimeMillis());
			notification.flags = Notification.FLAG_AUTO_CANCEL; 
			notification.tickerText = topic;
			Intent intent = null;
			if(callType == CallType.VIDEO) {
				intent = new Intent(CCPIntentUtils.ACTION_VIDEO_OUTCALL);
			} else {
				intent = new Intent(CCPIntentUtils.ACTION_VOIP_OUTCALL);
			}
			intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_NEW_TASK);  
			
			PendingIntent contentIntent = PendingIntent.getActivity(
					context, 
					R.string.app_name, 
					intent, 
					PendingIntent.FLAG_UPDATE_CURRENT);
			
			notification.setLatestEventInfo(context, 
					topic, 
					text, 
					contentIntent);
			
			mNotificationManager.notify(CCP_NOTIFICATOIN_ID_CALLING, notification);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	
	//voice .show notification ..
	public static void showNewMeidaMessageNoti(Context context ,IMChatMessageDetail rmsg) throws IOException {
		NotificationManager nm = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);               
		Notification n = new Notification(R.drawable.icon, rmsg.getSessionId() + context.getString(R.string.notifycation_new_message_recive)
				, System.currentTimeMillis());             
		n.flags = Notification.FLAG_AUTO_CANCEL;
		// 添加声音提示
		n.defaults = Notification.DEFAULT_SOUND;
		// audioStreamType的值必须AudioManager中的值，代表着响铃的模式 
		n.audioStreamType= android.media.AudioManager.ADJUST_LOWER;
		Intent intent = new Intent(context, GroupChatActivity.class);
		intent.putExtra(GroupBaseActivity.KEY_GROUP_ID, rmsg.getSessionId());
		
		
		intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_NEW_TASK);           
		PendingIntent contentIntent = PendingIntent.getActivity(
				context, 
		        R.string.app_name, 
		        intent, 
		        PendingIntent.FLAG_UPDATE_CURRENT);
		                 
		n.setLatestEventInfo(
				context,
				rmsg.getSessionId(), 
		        context.getString(R.string.notifycation_new_message_title), 
		        contentIntent);
		nm.notify(R.string.app_name, n);
		
	}
	//voice .show notification ..
	public static void showNewInterPhoneNoti(Context context ,String config) throws IOException {
		NotificationManager nm = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);               
		Notification n = new Notification(R.drawable.icon, context.getString(R.string.str_notifycation_inter_phone)
				, System.currentTimeMillis());             
		n.flags = Notification.FLAG_AUTO_CANCEL;
		n.defaults = Notification.DEFAULT_SOUND;
		n.audioStreamType= android.media.AudioManager.ADJUST_LOWER;
		Intent intent = new Intent(context, InterPhoneRoomActivity.class);
		intent.putExtra("confNo",config);
		intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_NEW_TASK);           
		//PendingIntent
		PendingIntent contentIntent = PendingIntent.getActivity(
				context, 
		        R.string.app_name, 
		        intent, 
		        PendingIntent.FLAG_UPDATE_CURRENT);
		                 
		n.setLatestEventInfo(
				context,
				config, 
		        context.getString(R.string.notifycation_new_inter_phone_title), 
		        contentIntent);
		nm.notify(R.string.app_name, n);
	}
	
	
	public static void cancleCCPNotification(Context context , int id) {
		checkNotification(context);
		
		mNotificationManager.cancel(id);
	}

	private static void checkNotification(Context context) {
		if(mNotificationManager == null) {
			mNotificationManager = (NotificationManager) context.getSystemService(
					Context.NOTIFICATION_SERVICE);
		}
	}
}
