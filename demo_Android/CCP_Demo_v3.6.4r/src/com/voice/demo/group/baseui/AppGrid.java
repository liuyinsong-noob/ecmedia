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
 */package com.voice.demo.group.baseui;

import com.voice.demo.tools.CCPUtil;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.GridView;
import android.widget.AdapterView;;

/**
 * <p>Title: CppAppGrid</p>
 * <p>Description: </p>
 * <p>Company: http://www.cloopen.com/</p>
 * @author  Jorstin Chan
 * @version 3.6
 * @date 2013-12-25
 */
public class AppGrid extends GridView implements AdapterView.OnItemClickListener{
	
	/**
	 * 
	 */
	private Context mContext;

	/**
	 * 
	 */
	private AppGirdApapter mGirdApapter;
	

	public AppGrid(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		mContext = context;
		initAppGird();
	}

	public AppGrid(Context context, AttributeSet attrs) {
		super(context, attrs);
		mContext = context;
		initAppGird();
	}

	public AppGrid(Context context) {
		super(context);
		mContext = context;
		initAppGird();
	}

	void initAppGird() {
		
		setBackgroundResource(0);
		setAdapter(mGirdApapter);
		setOnItemClickListener(this);
		
		int left = CCPUtil.getMetricsDensity(mContext, 10.0F);
		int top = CCPUtil.getMetricsDensity(mContext, 6.0F);
		setPadding(left, top, left, 0);
	}
	
	public int getCount() {
		return this.mGirdApapter.getCount() - 1;
	}
	
	
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
	}
	
	
	
}
