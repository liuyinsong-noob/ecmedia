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
package com.voice.demo.voip;

import com.CCP.phone.NativeInterface;
import com.hisun.phone.core.voice.Device.State;
import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;
import com.yuntongxun.ecsdk.core.voip.ViERenderer;

import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.RelativeLayout;

public class NetPhoneCallActivity extends CCPBaseActivity implements View.OnClickListener{
	public RelativeLayout mLoaclVideoView;
	private  boolean isRecordMP4 = false;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_person_info)
				, null);
		
		
		findViewById(R.id.netphone_landing_call).setOnClickListener(this);
		findViewById(R.id.netphone_voip_call).setOnClickListener(this);
		findViewById(R.id.netphone_landing_record_video_preview).setOnClickListener(this);
		// 添加小视频录制预览
		mLoaclVideoView = (RelativeLayout) findViewById(R.id.record_localvideo_view);
		SurfaceView localView = ViERenderer.CreateLocalRenderer(this);
		mLoaclVideoView.addView(localView);
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.netphone_voip_call:
			startActivity(new Intent(NetPhoneCallActivity.this, VoIPCallActivity.class));
			break;
		case R.id.netphone_landing_call:
			startActivity(new Intent(NetPhoneCallActivity.this, LandingCallActivity.class));
			break;
		case  R.id.netphone_landing_record_video_preview:

			Thread work = new Thread(new Runnable() {
				@Override
				public void run() {
					if(!isRecordMP4) {
						String sdcard_path = Environment.getExternalStorageDirectory().getPath()  + "/little_video_demo.mp4";
						NativeInterface.startRecordLocalMedia(sdcard_path , null);
						isRecordMP4 = true;
					} else {
						NativeInterface.stopRecordLocalMedia();
						isRecordMP4 = false;
					}

				}
			});
			work.start();
			break;
		default:
			break;
		}
	}
	
	public void checkUserState() {
		// Parameters: the user's VoIP account that need be query.
		// For example : 80175500000001
		State state = getDeviceHelper().checkUserOnline("");

		if(state == State.ONLINE) {
			// user online.
		} else if (state == State.OFFLINE) {
			// user offline
		} else if (state == State.NOTEXIST) {
			// account not found
		} else if (state == State.TIMEOUT) {
			// request time out.
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_netphone_talk_mode_activity;
	}
}
