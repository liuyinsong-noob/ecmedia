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
 */package com.voice.demo.ExConsultation;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.RatingBar;
import android.widget.TextView;

import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.ExConsultation.model.Expert;;

/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ExpertOrderActivity extends ExpertBaseActivity {
  
	

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.layout_layer_order_xh);
		
		TextView titleView = (TextView) findViewById(R.id.voice_title);
		titleView.setText(R.string.xh_lawyer_order_str);
		findViewById(R.id.voice_btn_back).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		Expert lawyer = (Expert) CCPApplication.getInstance().getData("lawyer");
		CCPApplication.getInstance().removeData("lawyer");
		updateLawyerUI(lawyer);
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
					if("1".equals(grade)){
						disp_grade = "实习医生";
						rate = 1.0f;
					}else if("2".equals(grade)){
						disp_grade = "医师";
						rate = 2.0f;
					}else if("3".equals(grade)){
						disp_grade = "主治医师";
						rate = 3.0f;
					}else if("4".equals(grade)){
						disp_grade = "副主任医师";
						rate = 4.0f;
					}else if("5".equals(grade)){
						disp_grade = "主任医师";
						rate = 5.0f;
					}else if("0".equals(grade)){
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
}
