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
 */package com.voice.demo.videoconference;

import android.os.Bundle;

import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;

import android.view.*;

public class VideoConference extends VideoconferenceBaseActivity implements View.OnClickListener{

	public static String videoConference = "conf400123450510002489";
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		findViewById(R.id.create).setOnClickListener(this);
		findViewById(R.id.querylist).setOnClickListener(this);
		findViewById(R.id.querymembers).setOnClickListener(this);
		findViewById(R.id.dismiss).setOnClickListener(this);
		findViewById(R.id.exit).setOnClickListener(this);
		findViewById(R.id.join).setOnClickListener(this);
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.create:
			getDeviceHelper().startVideoConference(CCPConfig.App_ID, "VideoConference", 8, "", "");
			break;
		case R.id.querylist:
			getDeviceHelper().queryVideoConferences(CCPConfig.App_ID, "");
			break;
		case R.id.querymembers:
			getDeviceHelper().queryMembersInVideoConference(videoConference);
			break;
		case R.id.dismiss:
			getDeviceHelper().dismissVideoConference(CCPConfig.App_ID, videoConference);
			break;
		case R.id.join:
			getDeviceHelper().joinVideoConference(videoConference);
		case R.id.exit:
			getDeviceHelper().exitVideoConference();
		default:
			break;
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_video_conference;
	}

}
