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

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;

import android.text.Editable;
import android.text.TextWatcher;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;
import android.widget.RelativeLayout;


import com.CCP.phone.NativeInterface;
import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;

/**
 * ChatRoom Converstion list ...
 *
 */
public class ChatroomConversation extends CCPBaseActivity implements View.OnClickListener{
	public RelativeLayout mLoaclVideoView;
	private  boolean running = false;
    private EditText ec_live_url_text;
    private String live_url_string;
    SurfaceView renderView;
    SharedPreferences.Editor live_url_editor;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_person_info)
				, null);

        SharedPreferences sharedPreferences = getSharedPreferences("ec_live_url_xml", Context.MODE_PRIVATE);
        // 获取Editor对象
        live_url_editor = sharedPreferences.edit();



        findViewById(R.id.netphone_landing_publish_video).setOnClickListener(this);
		findViewById(R.id.netphone_landing_switch_camera).setOnClickListener(this);
		findViewById(R.id.netphone_landing_play_video).setOnClickListener(this);

        //live video url
        ec_live_url_text = (EditText) findViewById(R.id.ec_live_url_text);
		mLoaclVideoView = (RelativeLayout) findViewById(R.id.cpp_live_video_view);
//		 renderView = ViERenderer.CreateLocalRenderer(this);

        renderView = new SurfaceView(this);
		mLoaclVideoView.addView(renderView);
		// NativeInterface.setAudioContext(getApplicationContext());
		NativeInterface.createLiveStream();


		getDeviceHelper().setVideoView("1007", renderView, null);

        live_url_string = sharedPreferences.getString("live_url", "rtmp://");

        ec_live_url_text.addTextChangedListener(new TextWatcher() {

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence text, int start, int before, int count) {
                live_url_editor.putString("live_url", ec_live_url_text.getText().toString());
                live_url_editor.commit();
            }

            @Override
            public void afterTextChanged(Editable s) {

            }
        });

        ec_live_url_text.setText(live_url_string);

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
							NativeInterface.pushLiveStream(ec_live_url_text.getText().toString(), null);
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
				Thread play_pthread = new Thread(new Runnable() {
					@Override
					public void run() {
                        if(!running) {
                            running = true;
                            NativeInterface.playLiveStream(ec_live_url_text.getText().toString(), "1007");
                        } else  {
                            running = false;
                            NativeInterface.stopLiveStream();
                            //NativeInterface.releaseLiveStream();
                        }

					}
				});
				play_pthread.start();
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

