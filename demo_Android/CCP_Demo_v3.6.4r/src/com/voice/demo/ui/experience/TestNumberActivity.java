package com.voice.demo.ui.experience;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPDrawableUtils;
import com.voice.demo.ui.CCPHelper;
import com.voice.demo.ui.LoginUIActivity;
import com.voice.demo.views.CCPButton;
import com.voice.demo.views.CCPTitleViewBase;

/**
 * 
 * @ClassName: TestNumberActivity.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-16
 * @version 3.6
 */
public class TestNumberActivity extends LoginUIActivity implements View.OnClickListener{

	private CCPButton mBuildConfrim;
	
	private CCPTitleViewBase mCcpTitleViewBase;

	private TextView rePhone;

	private LinearLayout testLayout1;

	private LinearLayout testLayout2;
	
	private int layoutId = -1;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		initLayoutTitleBar();
		
		initBuildLayoutView();
		
		initViewUI();
	}
	
	/**
	 * 
	 */
	private void initLayoutTitleBar() {
		mCcpTitleViewBase = new CCPTitleViewBase(this);
		mCcpTitleViewBase.setCCPTitleBackground(R.drawable.video_title_bg);
		mCcpTitleViewBase.setCCPTitleViewText(getString(R.string.app_title_build_number));
		mCcpTitleViewBase.setCCPBackImageButton(R.drawable.video_back_button_selector , this).setBackgroundDrawable(null);
	}
	
	/**
	 * 
	 */
	private void initBuildLayoutView() {
		rePhone = (TextView) findViewById(R.id.regist_phone);
		testLayout1 = (LinearLayout) findViewById(R.id.testnumber1);
		testLayout2 = (LinearLayout) findViewById(R.id.testnumber2);
		
		rePhone.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.mobile), null, null, null);
		
		mBuildConfrim = (CCPButton) findViewById(R.id.build_confrim);
		mBuildConfrim.setOnClickListener(this);
		mBuildConfrim.setBackgroundResource(R.drawable.video_blue_button_selector);
		mBuildConfrim.setImageResource(R.drawable.complete);
	}
	
	/**
	 * set test number in view
	 */
	private void initViewUI(){
		rePhone.setText(CCPConfig.mobile);
		
		if(!TextUtils.isEmpty(CCPConfig.test_number)) {
			String[] splitTestNumber = CCPConfig.test_number.split(",");
			
			ArrayList<String> arrays = new ArrayList<String>();
			for(String str : splitTestNumber) {
				if(CCPConfig.mobile != null && CCPConfig.mobile.equals(str)) {
					continue;
				}
				arrays.add(str);
			}
			splitTestNumber = arrays.toArray(new String[]{});
			if(splitTestNumber.length>0)
				setBuildNumber(testLayout1, splitTestNumber[0]);
			else{
				setBuildNumber(testLayout1, null);
			}
			if(splitTestNumber.length > 1) {
				setBuildNumber(testLayout2, splitTestNumber[1]);
			} else {
				setBuildNumber(testLayout2, null);
			}
			
		} else {
			
			setBuildNumber(testLayout1, null);
			setBuildNumber(testLayout2, null);
		}
	}
	
	
	private void setBuildNumber(LinearLayout layout , String phone) {
		TextView number = (TextView) layout.findViewById(R.id.textview);
		number.setCompoundDrawables(CCPDrawableUtils.getDrawables(this, R.drawable.mobile), null, null, null);
		TextView verifyFlag = (TextView) layout.findViewById(R.id.verify_flag);
		
		if(TextUtils.isEmpty(phone)) {
			verifyFlag.setText(R.string.str_un_building);
			verifyFlag.setTextSize(TypedValue.COMPLEX_UNIT_PX,getResources().getDimensionPixelSize(R.dimen.small_text_size));
		} else {
			verifyFlag.setText(R.string.str_verify);
		}
		number.setText(phone);
		layout.setTag(phone);
		layout.setOnClickListener(this);
		
	}


	@Override
	protected int getLayoutId() {
		return R.layout.binding_test_number;
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.testnumber1:
		case R.id.testnumber2:
			
			Intent intent = new Intent(TestNumberActivity.this, BuildingNumberActivity.class);
			if(v.getTag() instanceof String) {
				String phone = (String) v.getTag();
				intent.putExtra("phone", phone);
			}
			layoutId = v.getId();
			startActivityForResult(intent, getLayoutId());
			break;
		case R.id.title_btn4:
		case R.id.build_confrim:
			
			finish();
		default:
			break;
		}
	}
	

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			HideSoftKeyboard();
		}
		return super.onKeyDown(keyCode, event);
		
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		

		Log4Util.d(CCPHelper.DEMO_TAG ,"[TestNumberActivity] onActivityResult: requestCode=" + requestCode
				+ ", resultCode=" + resultCode + ", data=" + data);

		// If there's no data (because the user didn't select a number and
		// just hit BACK, for example), there's nothing to do.
		if (requestCode != getLayoutId() ) {
			if (data == null) {
				return;
			}
		} else if (resultCode != RESULT_OK) {
			Log4Util.d(CCPHelper.DEMO_TAG ,"[TestNumberActivity] onActivityResult: bail due to resultCode=" + resultCode);
			return;
		}
		
		if(data.hasExtra("phone")) {
			Bundle extras = data.getExtras();
			if (extras != null) {
				String phoneStr = extras.getString("phone");
				if(layoutId != -1 ) {
					String[] splitTestNumber = CCPConfig.test_number.split(",");
					ArrayList<String> arrays = new ArrayList<String>();
					for(String str : splitTestNumber) {
						if(CCPConfig.mobile != null && CCPConfig.mobile.equals(str)) {
							continue;
						}
						arrays.add(str);
					}
					splitTestNumber = arrays.toArray(new String[]{});
					if(layoutId==R.id.testnumber1){
						if(splitTestNumber.length==0||splitTestNumber.length==1){
							CCPConfig.test_number =CCPConfig.mobile+","+phoneStr+",";
						}else{
							CCPConfig.test_number =CCPConfig.mobile+","+phoneStr+","+splitTestNumber[1];
						}
					}else if(layoutId==R.id.testnumber2){
						if(splitTestNumber.length==0){
							CCPConfig.test_number =CCPConfig.mobile+","+phoneStr;
						}else{
							CCPConfig.test_number =CCPConfig.mobile+","+splitTestNumber[0]+","+phoneStr;
						}
					}
					setBuildNumber((LinearLayout)findViewById(layoutId), phoneStr);
				}
			}
		}
	}

}