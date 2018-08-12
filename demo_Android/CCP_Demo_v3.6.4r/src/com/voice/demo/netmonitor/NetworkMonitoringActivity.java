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
package com.voice.demo.netmonitor;

import java.util.Timer;
import java.util.TimerTask;

import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.UDPSocketUtil;
import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.*;
import android.widget.Button;
import android.widget.TextView;


public class NetworkMonitoringActivity extends CCPBaseActivity implements View.OnClickListener{

	private Timer timer;
	private UDPSocketUtil mSocketLayer = null;
	private Button tryAgain;
	private Button clearBuuton;
	private TextView udpSend;
	private TextView updReceive;
	private TextView losebacket;
	private TextView maxDelay;
	private TextView minDelay;
	private TextView avarageDelay;
	
	TimerTask task = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.str_setting_item_netcheck)
				, getString(R.string.btn_clear_all_text));
		
		
		mSocketLayer = new UDPSocketUtil();
		tryAgain = (Button)findViewById(R.id.try_again);
		tryAgain.setOnClickListener(this);
		clearBuuton = (Button)findViewById(R.id.clear);
		clearBuuton.setOnClickListener(this);
		udpSend = (TextView)findViewById(R.id.totlesend);
		updReceive = (TextView)findViewById(R.id.totlereceive);
		losebacket = (TextView)findViewById(R.id.lostrate);
		maxDelay = (TextView)findViewById(R.id.maxdelay);
		minDelay = (TextView)findViewById(R.id.mindelay);
		avarageDelay = (TextView)findViewById(R.id.avaragedelay);
		startTimer();
		
		mSocketLayer.start();
		tryAgain.setEnabled(false);
		clearBuuton.setEnabled(true);
	}

	Handler handler = new Handler() {
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case 1:
				if (timer != null) {
					updateUI(false);
				}
				break;
			}
			super.handleMessage(msg);
		}
	};

	private void startTimer() {
		task = new TimerTask() {
			public void run() {
				Message message = new Message();
				message.what = 1;
				handler.sendMessage(message);
			}
		};
		if(timer == null) {
			timer = new Timer();
			timer.schedule(task, 0, 1000);
		}
	}

	private void stopTimer() {
		if (timer != null) {
			timer.cancel();
			timer = null;
			task = null;
		}
	}
	private void updateUI(boolean isRest) {
		if(isRest) {
			udpSend.setText("总发包数(包)：0");
			updReceive.setText("总收报数(包)：0");
			losebacket.setText("丢报率(％)：0");
			maxDelay.setText("最大延时(ms)：0");
			minDelay.setText("最小延时(ms)：0");
			avarageDelay.setText("平均延时(ms)：0");
			return ;
		}
		Log4Util.i(CCPHelper.DEMO_TAG, "mSocketLayer.getSumDelay() = " + mSocketLayer.getSumDelay());
		udpSend.setText("总发包数(包)：" + mSocketLayer.getSendPacketCount());
		updReceive.setText("总收报数(包)：" + mSocketLayer.getRecePacketCount() );
		losebacket.setText("丢报率(％)：" + (mSocketLayer.getSendPacketCount() == 0 ? "0": (((float)(mSocketLayer.getSendPacketCount()-mSocketLayer.getRecePacketCount()) * 100)/(mSocketLayer.getSendPacketCount()))));
		maxDelay.setText("最大延时(ms)：" + mSocketLayer.getMaxDelay());
		minDelay.setText("最小延时(ms)："+mSocketLayer.getMinDelay() );
		avarageDelay.setText("平均延时(ms)："+(mSocketLayer.getSendPacketCount() == 0 ? "0":(((long)(mSocketLayer.getSumDelay()/mSocketLayer.getSendPacketCount())))));
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		mSocketLayer.stop();
		stopTimer();
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.try_again:
			startTimer();
			mSocketLayer.start();
			tryAgain.setEnabled(false);
			clearBuuton.setEnabled(true);
			updateUI(true);
			break;
		case R.id.clear:
			mSocketLayer.stop();
			updateUI(false);
			stopTimer();
			tryAgain.setEnabled(true);
			clearBuuton.setEnabled(false);
			break;
		default:
			break;
		}
	}
	
	
	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_RIGHT_ACTION) {
			mSocketLayer.stop();
			updateUI(true);
			stopTimer();
			tryAgain.setEnabled(true);
			clearBuuton.setEnabled(false);
		} else {
			super.handleTitleAction(direction);
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_udp_monitor_activity;
	}


}
