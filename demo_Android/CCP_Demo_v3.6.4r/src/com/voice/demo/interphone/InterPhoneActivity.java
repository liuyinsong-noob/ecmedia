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
package com.voice.demo.interphone;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

/**
 * Interphone Conversation ..
 *
 */
public class InterPhoneActivity extends CCPBaseActivity implements View.OnClickListener , OnItemClickListener{

	private ListView mInterPhone;
	
	private LinearLayout mVoiceEmpty;
	
	private ArrayList<String> mInterphoneIds = new ArrayList<String>();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.voip_btn_select_intercom_text)
				, getString(R.string.str_btn_launch_talkback));
		
		
		
		mInterPhone = (ListView) findViewById(R.id.interphone_list);
		mInterPhone.setOnItemClickListener(this);
		findViewById(R.id.begin_inter_phone).setOnClickListener(this);
		mVoiceEmpty = (LinearLayout) findViewById(R.id.voice_empty);
		mInterPhone.setEmptyView(mVoiceEmpty);
		
		if(CCPApplication.interphoneIds != null && !CCPApplication.interphoneIds.isEmpty()) {
			mInterphoneIds.addAll(CCPApplication.interphoneIds);
		}
		
		ipa = new InterPhoneAdapter(this, mInterphoneIds);
		mInterPhone.setAdapter(ipa);
		
		registerReceiver(new String[]{CCPIntentUtils.INTENT_RECIVE_INTER_PHONE});
	}

	@Override
	protected void onResume() {
		super.onResume();
	}

	@Override
	protected void handleTitleAction(int direction) {
		
		if(direction == TITLE_RIGHT_ACTION) {
			Intent intent = new Intent(InterPhoneActivity.this, InviteInterPhoneActivity.class);
			intent.putExtra("create_to", InviteInterPhoneActivity.CREATE_TO_INTER_PHONE_VOICE);
			startActivity(intent);
		} else {
			
			super.handleTitleAction(direction);
		}
	}
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.begin_inter_phone:
			handleTitleAction(TITLE_RIGHT_ACTION);
			break;
		default:
			break;
		}
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if(mInterphoneIds != null) {
			
			mInterphoneIds.clear();
			mInterphoneIds = null;
		}
	}
	
	class InterPhoneAdapter extends ArrayAdapter<String> {

		LayoutInflater mInflater;
		
		public InterPhoneAdapter(Context context,List<String> objects) {
			super(context, 0, objects);
			
			mInflater = getLayoutInflater();
		}
		
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			InterPhonerHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.intephone_list_item, null);
				holder = new InterPhonerHolder();
				
				holder.roomName = (TextView) convertView.findViewById(R.id.room_name);
			} else {
				holder = (InterPhonerHolder) convertView.getTag();
			}
			
			try {
				// do ..
				String item = getItem(position);
				if(!TextUtils.isEmpty(item)) {
					holder.roomName.setText(item) ;
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			
			return convertView;
		}
		
		
		class InterPhonerHolder {
			TextView roomName;
		}
		
	}


	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		if(ipa != null) {
			Intent intent = new Intent(InterPhoneActivity.this , InterPhoneRoomActivity.class);
			intent.putExtra("confNo", ipa.getItem(position));
			startActivity(intent) ;
		}
	}
	
	@Override
	protected void onReceiveBroadcast(Intent intent) {
		super.onReceiveBroadcast(intent);
		if (intent != null && CCPIntentUtils.INTENT_RECIVE_INTER_PHONE.equals(intent.getAction())) {
			if(ipa  != null ) {
				mInterphoneIds.clear();
				mInterphoneIds.addAll(CCPApplication.interphoneIds);
				Log4Util.d(CCPHelper.DEMO_TAG, "InterPhoneAdapter.notifyDataSetChanged()");
				ipa.notifyDataSetChanged() ;
			}
		}

	}
	
	// voip helper
	private InterPhoneAdapter ipa;

	@Override
	protected int getLayoutId() {
		return R.layout.layout_interphone_list_activity;
	}
}
