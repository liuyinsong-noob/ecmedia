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

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.ui.CCPHelper;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

/**
 * 
* <p>Title: CCPDotView</p>
* <p>Description: </p>
* <p>Company: http://www.cloopen.com/</p>
* @author  Jorstin Chan
* @version 3.6
* @date 2013-12-24
 */
@TargetApi(Build.VERSION_CODES.HONEYCOMB)
public class CCPDotView extends LinearLayout {

	/**
	 * The default count of CCPDotView.
	 */
	private int defaultCount = 9;
	
	public CCPDotView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	public CCPDotView(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public CCPDotView(Context context) {
		super(context);
	}

	public void setMaxCount(int count) {
		
		Log4Util.d(CCPHelper.DEMO_TAG, "CCPDotView.setMaxCount: " + count);
		this.defaultCount = count;
	}
	
	/**
	 * The total number of dot, namely the total number of pages
	 * @param count
	 */
	public void setDotCount(int count) {
		
		Log4Util.d(CCPHelper.DEMO_TAG, "CCPDotView.setDotCount: " + count);
		
		if(count < 0) {
			
			return ;
		}
		
		if(count > this.defaultCount) {
			Log4Util.e(CCPHelper.DEMO_TAG, "setDotCount large than max count :" + this.defaultCount);
			count = defaultCount;
		}
		
		removeAllViews();
		
		while(count != 0 ) {
			ImageView imageControl = (ImageView) View.inflate(getContext(), R.layout.ccppage_control_image, null);
			
			// drak_dot
			imageControl.setImageResource(R.drawable.dark_dot);
			addView(imageControl);
			
			count --;
		}
		
		// white_dot
		ImageView imageView = (ImageView) getChildAt(0);
		if(imageView != null) {
			imageView.setImageResource(R.drawable.white_dot);
		}
	}
 	
	
	/**
	 * Set the current sdotselected
	 * @param selecteDot
	 */
	public void setSelectedDot(int selecteDot) {

		Log4Util.d(CCPHelper.DEMO_TAG, "setSelectedDot:target index is : "
				+ selecteDot);

		if (selecteDot >= getChildCount()) {

			selecteDot = getChildCount() - 1;

		}
		Log4Util.e(CCPHelper.DEMO_TAG,
				"setSelectedDot:after adjust index is : " + selecteDot);
		for (int i = 0; i < getChildCount(); i++) {
			// drak_dot
			((ImageView) getChildAt(i)).setImageResource(R.drawable.dark_dot);
		}
		if (selecteDot < 0) {
			selecteDot = 0;
		}

		// white_dot
		ImageView localImageView = (ImageView) getChildAt(selecteDot);
		if (localImageView != null) {
			localImageView.setImageResource(R.drawable.white_dot);
		}
	}

	public int getDotCount() {
		return defaultCount;
	}

	
}
