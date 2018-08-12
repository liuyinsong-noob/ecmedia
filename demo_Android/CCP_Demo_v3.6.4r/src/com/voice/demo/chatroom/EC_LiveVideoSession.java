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
package com.voice.demo.chatroom;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;

import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RelativeLayout;


import com.CCP.phone.NativeInterface;
import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;
import com.yuntongxun.ecsdk.core.voip.ViERenderer;

/**
 * ChatRoom Converstion list ...
 *
 */
public class EC_LiveVideoSession extends CCPBaseActivity implements View.OnClickListener{
	public RelativeLayout mLoaclVideoView;
	private  boolean running = false;
    private EditText ec_live_url_text;
    private String live_url_string;
    SurfaceView renderView;
    SurfaceView local_renderView;
    SharedPreferences.Editor live_url_editor;

    Button  netphone_landing_publish_video;
    Button  netphone_landing_switch_camera;
    Button  netphone_landing_play_video;

    int camera_index = 0; // camera index, 0: back, 1: front
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_live_video_show)
				, null);

        SharedPreferences sharedPreferences = getSharedPreferences("ec_live_url_xml", Context.MODE_PRIVATE);
        // 获取Editor对象
        live_url_editor = sharedPreferences.edit();

        netphone_landing_publish_video = (Button) findViewById(R.id.netphone_landing_publish_video);
        netphone_landing_publish_video.setOnClickListener(this);
        netphone_landing_switch_camera = (Button) findViewById(R.id.netphone_landing_switch_camera);
        netphone_landing_switch_camera.setOnClickListener(this);
        netphone_landing_play_video = (Button)findViewById(R.id.netphone_landing_play_video);
        netphone_landing_play_video.setOnClickListener(this);

        //live video url
        ec_live_url_text = (EditText) findViewById(R.id.ec_live_url_text);
		mLoaclVideoView = (RelativeLayout) findViewById(R.id.cpp_live_video_view);
        local_renderView = ViERenderer.CreateLocalRenderer(this);

        renderView = new SurfaceView(this);
		NativeInterface.createLiveStream();
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

			    if(!running) {
                    netphone_landing_publish_video.setText("停止推流");
                    mLoaclVideoView.addView(local_renderView);
                } else {
                    netphone_landing_publish_video.setText("开始推流");
                    mLoaclVideoView.removeView(local_renderView);
                }

                if(!running) {
                    running = true;
                    NativeInterface.configLiveVideoStream(1, 15, 1, false);
                    NativeInterface.pushLiveStream(ec_live_url_text.getText().toString(), null);
                } else {
                    running = false;
                    NativeInterface.stopLiveStream();
                }

//				Thread work1 = new Thread(new Runnable() {
//					@Override
//					public void run() {
//						if(!running) {
//							running = true;
//							NativeInterface.pushLiveStream(ec_live_url_text.getText().toString(), null);
//						} else {
//							running = false;
//							NativeInterface.stopLiveStream();
//						}
//					}
//				});
//				work1.start();
				break;
			case  R.id.netphone_landing_switch_camera:

                NativeInterface.selectCameraLiveStream(camera_index);
                if(camera_index == 0) {
                    camera_index = 1;
                } else {
                    camera_index = 0;
                }
				break;
			case  R.id.netphone_landing_play_video:
                if(!running) {
                    netphone_landing_play_video.setText("停止观看");
                    mLoaclVideoView.addView(renderView);
                } else {
                    netphone_landing_play_video.setText("开始观看");
                    mLoaclVideoView.removeView(renderView);
                }
                if(!running) {

                    getDeviceHelper().setVideoView("1007", renderView, null);
                    running = true;
                    NativeInterface.playLiveStream(ec_live_url_text.getText().toString(), "1007");
                } else  {
                    running = false;
                    NativeInterface.stopLiveStream();
                }
//				Thread play_pthread = new Thread(new Runnable() {
//					@Override
//					public void run() {
//                        if(!running) {
//
//                            getDeviceHelper().setVideoView("1007", renderView, null);
//                            running = true;
//                            NativeInterface.playLiveStream(ec_live_url_text.getText().toString(), "1007");
//                        } else  {
//                            running = false;
//                            NativeInterface.stopLiveStream();
//                        }
//
//					}
//				});
//				play_pthread.start();
				break;
			default:
				break;
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.ccp_live_video;
	}

	@Override
    protected void onDestroy() {
        super.onDestroy();
        NativeInterface.stopLiveStream();
        NativeInterface.releaseLiveStream();
    }

}

