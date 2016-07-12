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
package com.voice.demo.outboundmarketing;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.cloopen.rest.sdk.CCPRestSDK;
import com.cloopen.rest.sdk.CCPRestSDK.BodyType;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.R;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.outboundmarketing.RestHelper.onRestHelperListener;
import com.voice.demo.outboundmarketing.model.LandingCall;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.CCPHelper;

// Processing marketing call number is called out, 
// according to the status of marketing results update UI.
public class MarketStateActivity extends CCPBaseActivity implements View.OnClickListener, onRestHelperListener
{	
	private TextView							headNoteTxt;
	private Button								finishButton;
	private ListView							outboundList;
	private ArrayList<String> 					phoneNumArray;
	private String				mAudioName;
	
	private MakingCallAdapter mCallAdapter;
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.str_market_state_head_text)
				, null);
		
		// Remove the transmission of telephone number from the intent .
		Intent intent = getIntent();
		Bundle bundle = intent.getExtras();
		phoneNumArray = bundle.getStringArrayList("Outbound_phoneNum");
		mAudioName = bundle.getString("audio_name");
		
		
		headNoteTxt = (TextView)findViewById(R.id.head_note_Txt);
		headNoteTxt.setText(getString(R.string.str_state_head_note, phoneNumArray.size()));
		
		finishButton = (Button)findViewById(R.id.btn_finish_outbound);
		finishButton.setOnClickListener(this);
		
		outboundList = (ListView)findViewById(R.id.market_state_list);
		
		mCallAdapter = new MakingCallAdapter(initMakingCallResource(phoneNumArray));
        outboundList.setAdapter(mCallAdapter);
        
        // Outgoing requests
        RestHelper.getInstance().setOnRestHelperListener(this);
        new LandingCallAsyncTask().execute();
	}
	
	public ArrayList<HashMap<String, Object>> getData()
	{
		ArrayList<HashMap<String, Object>> listItem = new ArrayList<HashMap<String, Object>>();
 
	    for(int i=0; i < phoneNumArray.size(); i++)
	    {
	    	HashMap<String, Object> map = new HashMap<String, Object>();
	    	
	    	if (i==0) {
		    	map.put("ItemImage", R.drawable.status_speaking);// Image resources ID
		    	map.put("ItemState", getString(R.string.str_market_state_answer_success));
		    	
			} else if (i==1) {
		    	map.put("ItemImage", R.drawable.status_quit);// Image resources ID
		    	map.put("ItemState", getString(R.string.str_market_state_other_busy));
		    	
			} else if (i==2) {
		    	map.put("ItemImage", R.drawable.status_join);// Image resources ID
		    	map.put("ItemState", getString(R.string.str_market_state_calling));
		    	
			} else {
		    	map.put("ItemImage", R.drawable.status_wait);// Image resources ID
		    	map.put("ItemState", getString(R.string.str_market_state_call_wait));
			}
	    	
	    	map.put("ItemNum", phoneNumArray.get(i));
	    	listItem.add(map);
	    }

        return listItem;
    }
	
	public ArrayList<LandingCall> initMakingCallResource(ArrayList<String> phoneNumber) {
		ArrayList<LandingCall> landingCallCache = new ArrayList<LandingCall>();
		for (String str : phoneNumber) {
			if(TextUtils.isEmpty(str)) {
				continue;
			}
			
			LandingCall landingCall = new LandingCall(str);
			landingCall.setCallStatuImage(R.drawable.status_wait);
			landingCall.setCallStatus(getString(R.string.str_market_state_call_wait));
			landingCallCache.add(landingCall);
		}
		
		return landingCallCache;
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_finish_outbound: // Complete
		{
			handleTitleAction(TITLE_LEFT_ACTION);
		}
			break;

		default:
			break;
		}
	}
	
	@Override
	public void onLandingCAllsStatus(final ERequestState reason, final LandingCall landingCall){
		
		if(landingCall == null ) {
			return;
		}
		Log4Util.d(CCPHelper.DEMO_TAG, landingCall.getPhoneNumber());
		getBaseHandle().post(new Runnable() {
			
			@Override
			public void run() {
				if(mCallAdapter != null) {
					for (int i = 0 ; i< mCallAdapter.getCount(); i ++) {
						LandingCall item = mCallAdapter.getItem(i);
						if(item != null && VoiceUtil.getStandardMDN(item.getPhoneNumber()).equals(landingCall.getPhoneNumber())) {
							
							if(reason == ERequestState.Success) {
								item.setCallStatuImage(R.drawable.status_speaking);
								item.setCallStatus(getString(R.string.str_market_state_answer_success));
							} else {
								item.setCallStatuImage(R.drawable.status_quit);
								item.setCallStatus("呼叫失败");
							}
							mCallAdapter.notifyDataSetChanged();
							return;
						}
					}
				}
			}
		});
	}
	
	@Override
	public void onVoiceCode(ERequestState reason) {

	}

	public final class ViewHolder {
		public ImageView stateImg;
		public TextView numTxt;
		public TextView stateTxt;
	}
	
	public class MakingCallAdapter extends ArrayAdapter<LandingCall>
	{
		
		LayoutInflater mInflater;
		
		public MakingCallAdapter(List<LandingCall> calls) {
			super(MarketStateActivity.this, 0, calls);
			
			mInflater = getLayoutInflater();
		}


		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			ViewHolder holder = null;
			
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_item_market_state, null);
				holder = new ViewHolder();
				
				holder.stateImg = (ImageView)convertView.findViewById(R.id.StateImg);
				holder.numTxt = (TextView)convertView.findViewById(R.id.StateNumTxt);
				holder.stateTxt = (TextView)convertView.findViewById(R.id.StateTxt);
				convertView.setTag(holder);	
			} else {
				holder = (ViewHolder)convertView.getTag();
			}
			
			try {
				LandingCall item = getItem(position);
				if(item != null) {
					holder.stateImg.setBackgroundResource(item.getCallStatuImage());
					holder.numTxt.setText(item.getPhoneNumber());
					holder.stateTxt.setText(item.getCallStatus());
				}
			} 
			catch (Exception e)
			{
				e.printStackTrace();
			}
			
			return convertView;
		}
	}
	
	public class LandingCallAsyncTask extends AsyncTask<Void, Void, String>
	{
		@Override
		protected String doInBackground(Void... params)
		{
			CCPRestSDK restAPI = new CCPRestSDK();
			restAPI.init(CCPConfig.REST_SERVER_ADDRESS, CCPConfig.REST_SERVER_PORT);
			restAPI.setAccount(CCPConfig.Main_Account, CCPConfig.Main_Token);
			restAPI.setAppId(CCPConfig.App_ID);
	        for (int i = 0; i < phoneNumArray.size(); i++)
			{
//	        	RestHelper.getInstance().LandingCalls(VoiceUtil.getStandardMDN(phoneNumArray.get(i)) , mAudioName , CCPConfig.App_ID , "");
	        	final String phone = phoneNumArray.get(i);
	        	HashMap<String,Object> landingCalls = restAPI.landingCall(phone, TextUtils.isEmpty(mAudioName)?"ccp_marketingcall.wav":mAudioName, "", "", "",1, "");
	        	Log.i("landingCalls", landingCalls.toString());
	        	final ERequestState state;
	        	 ERequestState mstate=null;
	        	if(landingCalls.containsKey("statusCode")&&"000000".equals(landingCalls.get("statusCode"))){
	        		mstate = ERequestState.Success;
	        	}else{
	        		mstate = ERequestState.Failed;
	        	}
	        	state=mstate;
	    		MarketStateActivity.this.runOnUiThread(new Runnable() {
	    			@Override
	    			public void run() {
	    				if(mCallAdapter != null) {
	    					for (int i = 0 ; i< mCallAdapter.getCount(); i ++) {
	    						LandingCall item = mCallAdapter.getItem(i);
	    						if(item != null && item.getPhoneNumber().equals(phone)) {
	    							
	    							if(state == ERequestState.Success) {
	    								item.setCallStatuImage(R.drawable.status_speaking);
	    								item.setCallStatus(getString(R.string.str_market_state_answer_success));
	    							} else {
	    								item.setCallStatuImage(R.drawable.status_quit);
	    								item.setCallStatus("呼叫失败");
	    							}
	    							mCallAdapter.notifyDataSetChanged();
	    							return;
	    						}
	    					}
	    				}
	    			}
	    		});
			}

			return null;
		}
	}

	@Override
	protected int getLayoutId() {
		return R.layout.layout_market_state_activity;
	}
}
