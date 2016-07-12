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
package com.voice.demo.setting;


import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import com.hisun.phone.core.voice.Build;
import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.ui.CCPBaseActivity;


public class SettingAboutActivity extends CCPBaseActivity implements View.OnClickListener{

	 
	

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.str_setting_item_abount)
				, null);
		initView();
	}




	private void initView() {
		((TextView)findViewById(R.id.demo_version)).setText("云通讯安卓客户端Demo V"+Build.SDK_VERSION);
		((TextView)findViewById(R.id.lib_version)).setText("LIB版本:"+Build.LIBVERSION.RELEASE);
		((TextView)findViewById(R.id.sdk_version)).setText("SDK版本:"+Build.SDK_VERSION);
		((TextView)findViewById(R.id.packagedate)).setText("打包日期:"+Build.LIBVERSION.COMPILE_DATE);
		((TextView)findViewById(R.id.serviceurl)).setText("服务器:"+CCPConfig.REST_SERVER_ADDRESS);
	}




	@Override
	protected void onDestroy() {
		super.onDestroy();
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		default:
			break;
		}
	}
	
	
	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_RIGHT_ACTION) {
		} else {
			super.handleTitleAction(direction);
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_setting_abountus;
	}


}
