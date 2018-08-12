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
package com.voice.demo.videoconference.baseui;

import com.voice.demo.R;
import com.voice.demo.tools.CCPUtil;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * 
* <p>Title: InstructionTipsView.java</p>
* <p>Description: New guidelines instruction</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-11-10
* @version 3.5
 */
public class InstructionTipsView extends LinearLayout {

	public InstructionTipsView(Context context) {
		super(context);
		initVideoInstruction(context);
	}

	public InstructionTipsView(Context context, AttributeSet attrs) {
		super(context, attrs);
		initVideoInstruction(context);
	}

	public InstructionTipsView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		
		initVideoInstruction(context);
	}

	private void initVideoInstruction(Context context) {
		setOrientation(LinearLayout.HORIZONTAL);
		
		LinearLayout.LayoutParams inTipsViewLayoutParams = new LinearLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT
				, FrameLayout.LayoutParams.WRAP_CONTENT);
		inTipsViewLayoutParams.gravity = Gravity.CENTER_VERTICAL;
		
		TextView iconTips = new TextView(context);
		iconTips.setLayoutParams(inTipsViewLayoutParams);
		
		Drawable drawableL = getResources().getDrawable(R.drawable.attention);
		drawableL.setBounds(0, 0, drawableL.getMinimumWidth(), drawableL.getMinimumHeight());
		
		Drawable drawableR = getResources().getDrawable(R.drawable.three_point);
		drawableR.setBounds(0, 0, drawableR.getMinimumWidth(), drawableR.getMinimumHeight());
		iconTips.setCompoundDrawables(drawableL, null, drawableR, null);

		iconTips.setCompoundDrawablePadding(CCPUtil.getMetricsDensity(context , 15.0F));
		iconTips.setText(getResources().getString(R.string.str_instruction_clicktips_1) + "\"");
		iconTips.setTextColor(Color.parseColor("#FFFFFF"));
		iconTips.setGravity(Gravity.CENTER);
		
		float scaledDensity = context.getResources().getDisplayMetrics().scaledDensity;
		iconTips.setTextSize(30/ scaledDensity);
		addView(iconTips);
		
		TextView textView2 = new TextView(context);
		textView2.setPadding(CCPUtil.getMetricsDensity(context , 15.0F), 0, 0, 0);
		textView2.setText("\"" + getResources().getString(R.string.str_instruction_clicktips_2));
		textView2.setTextColor(Color.parseColor("#FFFFFF"));
		textView2.setGravity(Gravity.CENTER);
		textView2.setTextSize(30/ scaledDensity);
		textView2.setLayoutParams(inTipsViewLayoutParams);
		
		addView(textView2);
	}

}
