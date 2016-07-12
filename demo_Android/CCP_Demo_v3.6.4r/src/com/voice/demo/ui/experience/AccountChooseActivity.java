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
 */package com.voice.demo.ui.experience;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;

import com.hisun.phone.core.voice.model.setup.SubAccount;
import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.tools.net.TaskKey;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CapabilityChoiceActivity;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.ui.model.DemoAccounts;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

/**
 * 
 * @ClassName: AccountChooseActivity 
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-3
 *
 */
public class AccountChooseActivity extends LoginUIActivity implements View.OnClickListener
																	,OnItemClickListener{

	CCPTitleViewBase mCcpTitleViewBase;
	
	private CCPButton mConfrim;
	
	private ListView mSubaccountListView;
	
	private boolean model;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		initLayoutTitleBar();
		
		initLayoutResource();
		
		
		if(CCPApplication.getInstance().getDemoAccounts() == null) {
			showConnectionProgress(getString(R.string.str_subaccount_obtaining));
			ITask iTask = new ITask(TaskKey.TASK_KEY_AUTO_LOGOIN);
			addTask(iTask);
		} else  {
			
			initListView();
		}
		
	}
	
	
	private void initLayoutTitleBar() {
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.app_title_experience_login));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
		mCcpTitleViewBase.setCCPActionImageButton(R.drawable.action_accountinfo_selector, this).setBackgroundDrawable(null);
	}
	
	
	private void initLayoutResource() {
		
		mConfrim = (CCPButton) findViewById(R.id.account_confrim);
		mConfrim.setOnClickListener(this);
		initImageButtonUI(false);
		
		mSubaccountListView = (ListView) findViewById(R.id.account_list);
		mSubaccountListView.setOnItemClickListener(this);
	}


	private void initImageButtonUI(boolean isChoice) {
		if(isChoice) {
			mConfrim.setBackgroundResource(R.drawable.video_blue_button_selector);
			mConfrim.setImageResource(R.drawable.login);
		} else {
			mConfrim.setText(R.string.str_no_choice_account);
			mConfrim.setTextSize(getResources().getDimensionPixelSize(R.dimen.ccp_button_text_size));
			ColorStateList colors = (ColorStateList) getResources().getColorStateList(R.color.ccp_attentoin_color); 
			mConfrim.setTextColor(colors);
			mConfrim.setBackgroundResource(R.drawable.ccp_no_enabled);
		}
		
		mConfrim.setEnabled(isChoice);
	}
	
	private List<String> initVoIPData() {
		String[] mInviterMember = null;
		if( CCPConfig.VoIP_ID_LIST != null) {
			mInviterMember = CCPConfig.VoIP_ID_LIST.split(",");
			if(mInviterMember == null || mInviterMember.length == 0) {
				throw new IllegalArgumentException("Load the VOIP account information errors" +
						", configuration information can not be empty" + mInviterMember);
			}
		}
		if(mInviterMember==null){
			throw new IllegalArgumentException("Load the VOIP account information errors" +
					", configuration information can not be empty" + mInviterMember);
		}
		List<String> objects = (List<String>) Arrays.asList(mInviterMember);
		return objects= model == false ? objects :CCPUtil.removeString(objects, CCPConfig.VoIP_ID);
		
	}
	
	
	private void initListView() {

		List<String> objects = initVoIPData();
		Collections.sort(objects);
		SubaccountAdapter adapter = new SubaccountAdapter(this, objects);
		mSubaccountListView.setAdapter(adapter);
	}


	@Override
	protected int getLayoutId() {
		return R.layout.ccp_account_choose;
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.title_btn4:
			CCPUtil.exitCCPDemo();
			finish();
			break;
		case R.id.title_btn1:
			startActivity(new Intent(AccountChooseActivity.this , ExperienceAccountInfoActivity.class));
			
			break;
			
		case R.id.account_confrim:
			
			Object tag = v.getTag();
			if(tag instanceof String) {
				
				initRegistConfig((String)tag);
				
				doSDKRegist();
			}
			break;
		default:
			break;
		}
	}

	@Override
	protected void handleClientLoginRequest() {
		super.handleClientLoginRequest();
		closeConnectionProgress();//to be added here while login finish
		initListView();
	}
	
	@Override
	protected void handleRequestFailed(int requestType, int errorCode,
			String errorMessage) {
		super.handleRequestFailed(requestType, errorCode, errorMessage);
		closeConnectionProgress();//
		startActivity(new Intent(this , ExperienceLogin.class));
		finish();
	}
	/**
	 * 
	 * @ClassName: VideoConferenceConvAdapter 
	 * @Description: TODO
	 * @author Jorstin Chan 
	 * @date 2013-12-3
	 *
	 */
	class SubaccountAdapter extends ArrayAdapter<String> {

		LayoutInflater mInflater;
		
		HashMap<Integer, Boolean> isSelected;
		
		public SubaccountAdapter(Context context, List<String> objects) {
			super(context, 0, objects);
			
			mInflater = getLayoutInflater();
			
			init(objects.size());
		}
		
		// initialize all checkbox are not selected
		public void init(int size) {
			if(isSelected!=null){
				isSelected.clear();
			}else{
				isSelected = new HashMap<Integer, Boolean>();
			}
			for (int i = 0; i < size; i++) {
				isSelected.put(i, false);
			}
		}
		
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			SubaccountHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.invite_member_list_item, null);
				holder = new SubaccountHolder();
				
				holder.number = (TextView) convertView.findViewById(R.id.name);
				holder.checkBox = (CheckBox) convertView.findViewById(R.id.check_box);
				
				convertView.setBackgroundResource(R.drawable.background_black);
				// set Video Conference Item Style
				ColorStateList colors = (ColorStateList) getResources().getColorStateList(R.color.white); 
				holder.number.setTextColor(colors);
				holder.checkBox.setButtonDrawable(R.drawable.ccp_checkbox);
			} else {
				holder = (SubaccountHolder) convertView.getTag();
			}
			
			try {
				// do ..
				String account = getItem(position);
				if(account != null ) {
					
					if(!TextUtils.isEmpty(account)) {
						holder.number.setText(account);
						holder.checkBox.setChecked(isSelected.get(position));
					}
				}
				
			} catch (Exception e) {
				e.printStackTrace();
			}
			
			return convertView;
		}
		
		
		class SubaccountHolder {
			TextView number;
			CheckBox checkBox;
		}
		
		public HashMap<Integer, Boolean> getIsSelected() {
			return isSelected;
		}
		
	}
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		
		if(iTask.getKey() == TaskKey.TASK_KEY_AUTO_LOGOIN) {
			
			String userName = CcpPreferences.getSharedPreferences().getString(
					CCPPreferenceSettings.SETTING_USERNAME_MAIL.getId(),
					(String) CCPPreferenceSettings.SETTING_USER_PASSWORD
							.getDefaultValue());
			
			String userpas = CcpPreferences.getSharedPreferences().getString(
					CCPPreferenceSettings.SETTING_USER_PASSWORD.getId(),
					(String) CCPPreferenceSettings.SETTING_USER_PASSWORD
					.getDefaultValue());
			
			doExperienceAutoLogin(userName, userpas);
			
//			closeConnectionProgress();//to be added here?
		}
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		if(parent.getAdapter() instanceof SubaccountAdapter) {
			SubaccountAdapter adapter = (SubaccountAdapter) parent.getAdapter();
				
			CheckBox cBox = (CheckBox) view.findViewById(R.id.check_box);
			if(!cBox.isChecked()) {
				adapter.init(adapter.getCount());
			}
			cBox.toggle();
			
			boolean isChoice = cBox.isChecked();
			adapter.getIsSelected().put(position, isChoice);
			initImageButtonUI(isChoice);
			adapter.notifyDataSetChanged();
			
			// set tag
			mConfrim.setTag(adapter.getItem(position));
		}
		
		
	}
	
	public void initRegistConfig(String sid) {
		
		if(TextUtils.isEmpty(sid)) {
			return;
		}
		DemoAccounts demoAccounts = CCPApplication.getInstance().getDemoAccounts();
		ArrayList<SubAccount> subAccounts = demoAccounts.getApplications().get(0).getSubAccounts();
		for(SubAccount account : subAccounts) {
			if(sid.equals(account.sipCode)) {
				
				CCPConfig.VoIP_ID = account.sipCode;
				CCPConfig.VoIP_PWD = account.sipPwd;
				CCPConfig.Sub_Account = account.accountSid;
				CCPConfig.Sub_Token = account.authToken;
				return;
			}
		}
	}
	
	/* (non-Javadoc)
	 * @see com.voice.demo.ui.LoginUIActivity#startAction()
	 */
	@Override
	protected void startAction() {
		super.startAction();
		// Confirmation Information,then send to next activity ,.  
		if (!CCPConfig.check()) {
			CCPApplication.getInstance().showToast(R.string.config_error_text);
			return;
		}
		Intent intent = new Intent();
		intent.setClass(AccountChooseActivity.this, CapabilityChoiceActivity.class);
		startActivity(intent);
		mConfrim.setEnabled(true);
		this.finish();
	}
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
			CCPUtil.exitCCPDemo();
			finish();
			return true;
		}

		return super.onKeyDown(keyCode, event);
	}
}
