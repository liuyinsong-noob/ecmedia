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
package com.voice.demo.ExConsultation;


import android.os.Bundle;
import android.text.Html;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RatingBar;
import android.widget.TextView;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.ExConsultation.model.Expert;
import com.voice.demo.tools.CCPIntentUtils;


/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ExpertDetailActivity extends ExpertBaseActivity implements OnClickListener{

	private Button mConsult;   //立即咨询
	private Button mAppointment; //预约咨询
	private Expert expert;
	private String cname = "";
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_layer_detail_xh);
		
		//handleTitleDisplay(getString(R.string.btn_title_back), getString(R.string.xh_lawyer_detail_str), null);
		TextView titleView = (TextView) findViewById(R.id.voice_title);
		titleView.setText(R.string.xh_lawyer_detail_str);
		findViewById(R.id.voice_btn_back).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		
		TextView mExpertiseArea = (TextView) findViewById(R.id.EditText_item_number_input);
		Spanned fromHtml = Html.fromHtml(getString(R.string.xh_special));
		mExpertiseArea.setText(fromHtml);
		
		TextView mPersonInfo = (TextView) findViewById(R.id.EditText_tem_city_result);
		Spanned fromHtml2 = Html.fromHtml(getString(R.string.xh_personal_info));
		mPersonInfo.setText(fromHtml2);
		
		
		expert = (Expert) CCPApplication.getInstance().getData("lawyer");
		cname = getIntent().getStringExtra("cname");
		CCPApplication.getInstance().removeData("lawyer");
		initResourceRefs();
		updateLawyerUI(expert);
	}

	private void updateLawyerUI(Expert expert){
		if(expert != null){
			ImageView head_icon = (ImageView) findViewById(R.id.xh_edit_photo);
			head_icon.setImageResource(expert.getHead_icon_id());
			TextView name_text = (TextView) findViewById(R.id.xh_personal_name);
			name_text.setText(expert.getName());
			TextView grade_view = (TextView) findViewById(R.id.xh_layer_levl);
			RatingBar bar = (RatingBar) findViewById(R.id.xh_layer_rating);
			String grade = expert.getGrade();
			String disp_grade = "";
			float rate = 0.0f;
				if(!TextUtils.isEmpty(grade)){
					if("0".equals(grade)){
						disp_grade = "实习医生";
						rate = 1.0f;
					}else if("1".equals(grade)){
						disp_grade = "医师";
						rate = 2.0f;
					}else if("2".equals(grade)){
						disp_grade = "主治医师";
						rate = 3.0f;
					}else if("3".equals(grade)){
						disp_grade = "副主任医师";
						rate = 4.0f;
					}else if("4".equals(grade)){
						disp_grade = "主任医师";
						rate = 5.0f;
					}else if("5".equals(grade)){
						disp_grade = "专家";
					}else {
						disp_grade = "老专家";
						rate = 5.0f;
					}
				grade_view.setText(disp_grade);
				bar.setRating(rate);
			}
			TextView detail = (TextView) findViewById(R.id.xh_layer_speciality);
            if(!TextUtils.isEmpty(expert.getDetail())){
            	detail.setText(expert.getDetail());
            }else{
            	//detail.setText("专长：" + cname);
            }
		}
	}

	private void initResourceRefs() {
		mConsult = (Button) findViewById(R.id.xh_consult);
		mAppointment = (Button) findViewById(R.id.xh_appointment);
		mConsult.setOnClickListener(this);
		mAppointment.setOnClickListener(this);
		
	}


	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.xh_consult:
			CCPApplication.getInstance().putData("lawyer", expert);
			startAction(CCPIntentUtils.ACTION_EXPERT_CONMUI);
					break;
		case R.id.xh_appointment:
			CCPApplication.getInstance().putData("lawyer", expert);
			startAction(CCPIntentUtils.ACTION_EXPERT_ORDER, "cname", cname);
			break;

		default:
			break;
		}
	}
}
