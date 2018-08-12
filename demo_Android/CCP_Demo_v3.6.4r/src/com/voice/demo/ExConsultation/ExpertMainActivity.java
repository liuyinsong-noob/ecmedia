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

import java.util.ArrayList;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.voice.demo.R;
import com.voice.demo.ExConsultation.model.Category;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.CCPIntentUtils;
import com.voice.demo.tools.net.ITask;


/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ExpertMainActivity extends ExpertBaseActivity  implements OnClickListener, OnItemClickListener{

	private LayoutInflater inflater;
	private ArrayList<Category> category_list;
	private GridView gridview;
	private GridViewAdapter gridAdapter;


	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.laout_main_xh);
		gridview = (GridView) findViewById(R.id.gridview);
		gridview.setHorizontalSpacing(50);
		gridview.setOnItemClickListener(this);
		
		View empty = findViewById(R.id.error_tip);
		gridview.setEmptyView(empty);
		//
		showConnectionProgress(getString(R.string.progress_text_1));
		ITask iTask = new ITask(ExpertManagerHelper.KEY_CATEGORY_LIST);
		addTask(iTask);
		
		//startLoading();
		inflater = getLayoutInflater();
		
		/*handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.xh_apptitle_expert_information)
				, getString(R.string.xh_tilte_right_button_admin));*/
		TextView titleView = (TextView) findViewById(R.id.voice_title);
		titleView.setText(R.string.xh_apptitle_expert_information);
		ImageButton rImageButton = (ImageButton) findViewById(R.id.voice_right_btn);
		rImageButton.setImageResource(R.drawable.setting_icon);
		rImageButton.setVisibility(View.INVISIBLE);
		findViewById(R.id.voice_btn_back).setOnClickListener(this);
	}

	
	@Override
	protected void handleTitleAction(int direction) {
		if(TITLE_RIGHT_ACTION == direction) {
			
			// mamager /...
		} else {
			super.handleTitleAction(direction);
		}
	}
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.voice_btn_back:
			this.finish();
			break;
		case R.id.voice_right_btn:

			break;
		default:
			break;
		}
	}
	

	final class GridViewAdapter extends BaseAdapter {
		
		@Override
		public int getCount() {
			return category_list.size();
		}

		@Override
		public Object getItem(int position) {
			return category_list.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			CategoryHolder holder = null;
			if(convertView == null) {
				convertView = inflater.inflate(R.layout.xh_grid_item_layout, null);
				holder = new CategoryHolder();
				holder.category_name = (TextView) convertView.findViewById(R.id.xh_ItemText);
				holder.category_view = (ImageView) convertView.findViewById(R.id.xh_ItemImage);
				convertView.setTag(holder);
			} else{
				holder = (CategoryHolder) convertView.getTag();
			}
			Category category = category_list.get(position);
			holder.category_name.setText(category.getCategory_name());
			if(position%18 == 0){
				holder.category_view.setImageResource(R.drawable.clinic_01_icon);
			}else if(position%18 == 1){
				holder.category_view.setImageResource(R.drawable.clinic_02_icon);
			}else if(position%18 == 2){
				holder.category_view.setImageResource(R.drawable.clinic_03_icon);
			}else if(position%18 == 3){
				holder.category_view.setImageResource(R.drawable.clinic_04_icon);
			}else if(position%18 == 4){
				holder.category_view.setImageResource(R.drawable.clinic_05_icon);
			}else if(position%18 == 5){
				holder.category_view.setImageResource(R.drawable.clinic_06_icon);
			}else if(position%18 == 6){
				holder.category_view.setImageResource(R.drawable.clinic_07_icon);
			}else if(position%18 == 7){
				holder.category_view.setImageResource(R.drawable.clinic_08_icon);
			}else if(position%18 == 8){
				holder.category_view.setImageResource(R.drawable.clinic_09_icon);
			}else if(position%18 == 9){
				holder.category_view.setImageResource(R.drawable.clinic_10_icon);
			}else if(position%18 == 10){
				holder.category_view.setImageResource(R.drawable.clinic_11_icon);
			}else if(position%18 == 11){
				holder.category_view.setImageResource(R.drawable.clinic_12_icon);
			}else if(position%18 == 12){
				holder.category_view.setImageResource(R.drawable.clinic_13_icon);
			}else if(position%18 == 13){
				holder.category_view.setImageResource(R.drawable.clinic_14_icon);
			}else if(position%18 == 14){
				holder.category_view.setImageResource(R.drawable.clinic_15_icon);
			}else if(position%18 == 15){
				holder.category_view.setImageResource(R.drawable.clinic_16_icon);
			}else if(position%18 == 16){
				holder.category_view.setImageResource(R.drawable.clinic_17_icon);
			}else if(position%18 == 17){
				holder.category_view.setImageResource(R.drawable.clinic_18_icon);
			}
			
			return convertView;
		}

	    class CategoryHolder {
			public ImageView category_view;
			public TextView category_name;
		}
		
	}
	
	
	@Override
	protected void handleGetClientGategory(ERequestState reason,
			ArrayList<Category> xcCategories) {
		super.handleGetClientGategory(reason, xcCategories);
		if(reason == ERequestState.Success) {
			category_list = xcCategories;
			if(category_list != null && category_list.size() > 0){
				if(gridAdapter == null){
					gridAdapter = new GridViewAdapter();
					gridview.setAdapter(gridAdapter);
				}else {
					gridAdapter.notifyDataSetChanged();
				}
			}
		} else {
			gridview.setAdapter(null);
		}
		closeConnectionProgress();
	}
	
	
	@Override
	protected void handleTaskBackGround(ITask iTask) {
		super.handleTaskBackGround(iTask);
		int key = iTask.getKey();
		if(key == ExpertManagerHelper.KEY_CATEGORY_LIST) {
			ExpertManagerHelper.getInstance().getCategoryList();
		}
	} 

	@Override
	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long arg3) {
		Category category = category_list.get(position);
		Intent intent = new Intent(CCPIntentUtils.ACTION_EXPERT_LIST_VIEW);
		intent.putExtra("cld", category.getCategory_id());
		intent.putExtra("cname", category.getCategory_name());
		startActivity(intent);
	}
	
}
