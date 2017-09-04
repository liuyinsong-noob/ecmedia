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
package com.voice.demo.chatroom;

import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;

import android.view.SurfaceView;
import android.view.View;
import android.widget.RelativeLayout;


import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.Device;
import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.voip.LandingCallActivity;
import com.voice.demo.voip.NetPhoneCallActivity;
import com.voice.demo.voip.VoIPCallActivity;
import com.yuntongxun.ecsdk.core.voip.ViERenderer;

/**
 * ChatRoom Converstion list ...
 *
 */
public class ChatroomConversation extends CCPBaseActivity implements View.OnClickListener{
	public RelativeLayout mLoaclVideoView;
	private  boolean running = false;

	 SurfaceView localView;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_person_info)
				, null);


		findViewById(R.id.netphone_landing_publish_video).setOnClickListener(this);
		findViewById(R.id.netphone_landing_switch_camera).setOnClickListener(this);
		findViewById(R.id.netphone_landing_play_video).setOnClickListener(this);

		mLoaclVideoView = (RelativeLayout) findViewById(R.id.cpp_live_video_view);
//		 localView = ViERenderer.CreateLocalRenderer(this);

		 localView = new SurfaceView(this);



		mLoaclVideoView.addView(localView);
		 NativeInterface.setAudioContext(getApplicationContext());
		NativeInterface.createLiveStream();


	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
			case  R.id.netphone_landing_publish_video:
				Thread work1 = new Thread(new Runnable() {
					@Override
					public void run() {
						if(!running) {
							running = true;
							NativeInterface.pushLiveStream("rtmp://192.168.0.44/live/livestream", null);
						} else {
							running = false;
							NativeInterface.stopLiveStream();
						}
					}
				});
				work1.start();
				break;
			case  R.id.netphone_landing_switch_camera:
				break;
			case  R.id.netphone_landing_play_video:
				Thread work = new Thread(new Runnable() {
					@Override
					public void run() {
						 NativeInterface.playLiveStream("rtmp://192.168.0.44/live/livestream", localView);

					}
				});
				work.start();
				break;
			default:
				break;
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.ccp_live_video;
	}
}

