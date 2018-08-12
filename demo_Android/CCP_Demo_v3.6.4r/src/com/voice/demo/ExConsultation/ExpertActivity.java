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
package com.voice.demo.ExConsultation;

import java.util.ArrayList;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RatingBar;
import android.widget.TextView;

import com.voice.demo.R;
import com.voice.demo.CCPApplication;
import com.voice.demo.ExConsultation.model.Expert;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.net.ITask;


/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ExpertActivity extends ExpertBaseActivity implements OnItemClickListener {

	private ListView mLayerView;
	private ArrayList<Expert>  mExpertList;
	private LayerListAdaper layerAdaper;
	private String category_name = "";
	@Override
	protected void handleGetExpertClassic(ERequestState reason,
			ArrayList<Expert> xExperts) {
		super.handleGetExpertClassic(reason, xExperts);
		if(reason == ERequestState.Success) {
			mExpertList = xExperts;
	         if(mExpertList != null && mExpertList.size() > 0){
	        	 if(layerAdaper == null){
	        		layerAdaper = new LayerListAdaper();
	        		mLayerView.setAdapter(layerAdaper);
	        	 }else{
	        		 layerAdaper.notifyDataSetChanged();
	        	 }
	         }
		} else {
			mLayerView.setAdapter(null);
		}
		closeConnectionProgress();
	}
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == ExpertManagerHelper.KEY_EXPERT_LIST) {
			String cld = (String) iTask.getTaskParameters("cld");
			ExpertManagerHelper.getInstance().getExpertList(cld);
		}
	} 
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.layout_lawyer_list_xh);
		
		
		TextView titleView = (TextView) findViewById(R.id.voice_title);
		findViewById(R.id.voice_btn_back).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				finish();
			}
		});
				
		String cld = getIntent().getStringExtra("cld");
		category_name = getIntent().getStringExtra("cname");
        showConnectionProgress(getString(R.string.progress_text_1));
		ITask iTask = new ITask(ExpertManagerHelper.KEY_EXPERT_LIST);
		iTask.setTaskParameters("cld", cld);
		addTask(iTask);
		
		
		titleView.setText(getString(R.string.xh_lawyer_list_str ,category_name));

		
		initResourceRefs();
		layerAdaper.notifyDataSetChanged();
		
		
	}
	

	private void initResourceRefs() {
		mLayerView = (ListView) findViewById(R.id.xh_layer_list);
		layerAdaper = new LayerListAdaper();
		mLayerView.setAdapter(layerAdaper);
		mLayerView.setOnItemClickListener(this);
		
		View view = findViewById(R.id.error_tip);
		mLayerView.setEmptyView(view);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if(mExpertList != null){
			mExpertList.clear();
		}
	}
	
	//layer adapter
	class LayerListAdaper extends BaseAdapter{

		@Override
		public int getCount() {
			if(mExpertList != null){
				return mExpertList.size();
			}
			return 0;
		}

		@Override
		public Object getItem(int position) {
			if(mExpertList != null){
				return mExpertList.get(position);
			}
			return null;
		}

		@Override
		public long getItemId(int position) {
			if(mExpertList != null){
				return position;
			}
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			LayerHolder holder;
			if (convertView == null || convertView.getTag() == null) {
				convertView = getLayoutInflater().inflate(
						R.layout.xh_list_item_lawyer, null);
				holder = new LayerHolder();
				holder.layerName = (TextView) convertView
						.findViewById(R.id.xh_personal_name);
				holder.layer_icon = (ImageView) convertView.findViewById(R.id.xh_edit_photo);
				holder.layer_detail = (TextView) convertView.findViewById(R.id.xh_layer_speciality);
			    holder.layer_grade = (TextView) convertView.findViewById(R.id.xh_layer_levl);
			    holder.bar = (RatingBar) convertView.findViewById(R.id.xh_layer_rating);
				convertView.setTag(holder);
			} else {
				holder = (LayerHolder) convertView.getTag();
			}
			
			Expert expert = mExpertList.get(position);
			if(!TextUtils.isEmpty(expert.getName())){
				holder.layerName.setText(expert.getName());
			}else{
				holder.layerName.setText("未命名");
			}
			int resid = R.drawable.head_portrait;
			if(position %2 == 0){
				holder.layer_icon.setImageResource(R.drawable.head_portrait);
				 resid = R.drawable.head_portrait;
			}else if(position %2 == 1){
				holder.layer_icon.setImageResource(R.drawable.head_portrait_default);
				 resid = R.drawable.head_portrait_default;
			}
			expert.setHead_icon_id(resid);
			
			String grade = expert.getGrade();
			float rate = 0.0f;
			String disp_grade = "";
			if(!TextUtils.isEmpty(grade)){
				if("0".equals(grade)){
					disp_grade = "实习医生";
					rate = 0.0f;
				}else if("1".equals(grade)){
					disp_grade = "医师";
					rate = 1.0f;
				}else if("2".equals(grade)){
					disp_grade = "主治医师";
					rate = 2.0f;
				}else if("3".equals(grade)){
					disp_grade = "副主任医师";
					rate = 3.0f;
				}else if("4".equals(grade)){
					disp_grade = "主任医师";
					rate = 4.0f;
				}else if("5".equals(grade)){
					disp_grade = "专家";
					rate = 5.0f;
				}else {
					disp_grade = "老专家";
					rate = 5.0f;
				}
				holder.layer_grade.setText(disp_grade);
				holder.bar.setRating(rate);
			}else{
				 holder.layer_grade.setText("未设置等级");
			     holder.bar.setRating(rate);
			}
			if(!TextUtils.isEmpty(expert.getDetail())){
				holder.layer_detail.setText(expert.getDetail());	
			}else{
				//holder.layer_detail.setText("专长：" + category_name);
			}

			return convertView;
		}
		
		class LayerHolder {
			TextView layerName;
			ImageView layer_icon;
			TextView  layer_grade;
			TextView layer_detail;
			RatingBar bar;
		}
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		Expert expert =  mExpertList.get(position);
		startAction(CCPIntentUtils.ACTION_EXPERT_DETAIL_VIEW,"cname",category_name);
		CCPApplication.getInstance().putData("lawyer", expert);
	}
	
}
