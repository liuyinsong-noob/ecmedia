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
package com.voice.demo.ui.preference;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;

import com.voice.demo.ui.CCPBaseActivity;

/**
 * 
 * @ClassName: CCPPreference.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-10
 * @version 3.6
 */
public class CCPPreference extends CCPBaseActivity {
	
	private SharedPreferences mCCPSPreferences;
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#onCreate(android.os.Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		mCCPSPreferences = getSharedPreferences(getPackageName() + "_preferences", Context.MODE_PRIVATE);
	}
	

	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#getLayoutId()
	 */
	@Override
	protected int getLayoutId() {
		return 0;
	}

}
