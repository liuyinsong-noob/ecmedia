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
package com.voice.demo.videoconference;

import java.io.InvalidClassException;

import com.voice.demo.R;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;

import android.content.Intent;
import android.os.Bundle;
import android.text.Html;
import android.text.Spanned;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

/**
 * 
* <p>Title: WizardVideoConfActivity.java</p>
* <p>Description: Video conference entrance interface.
* Introduction video conferencing and application scenarios</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-29
* @version 3.5
 */
public class WizardVideoconferenceActivity extends VideoconferenceBaseActivity {

	private int videoType;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		videoType = getIntent().getIntExtra(VideoconferenceConversation.CONFERENCE_TYPE, 0);
		if(videoType == 1) {
			doActoin();
			finish();
			return ;
		}
		
		TextView mVideoDesc = (TextView) findViewById(R.id.video_desc);
		Spanned fromHtml = Html.fromHtml(getString(R.string.str_video_desc));
		mVideoDesc.setText(fromHtml);
		
		Button button = (Button) findViewById(R.id.start);
		button.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				doActoin();
			}

		});
		
		boolean first = CcpPreferences.getSharedPreferences().getBoolean(CCPPreferenceSettings.SETTINGS_VIDEO_CONF_FIRST.getId(), (Boolean)CCPPreferenceSettings.SETTINGS_VIDEO_CONF_FIRST.getDefaultValue());
		
		if(first) {
			 try {
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTINGS_VIDEO_CONF_FIRST, true, true);
			} catch (InvalidClassException e) {
				e.printStackTrace();
			}
			button.performClick();
		}
		
		
	}
	
	/**
	 * 
	 */
	private void doActoin() {
		Intent intent = new Intent(WizardVideoconferenceActivity.this, VideoconferenceConversation.class);
		intent.putExtra(VideoconferenceConversation.CONFERENCE_TYPE, videoType);
		WizardVideoconferenceActivity.this.startActivity(intent);
		WizardVideoconferenceActivity.this.finish();
	}

	@Override
	protected int getLayoutId() {
		return R.layout.video_wizard;
	}
}
