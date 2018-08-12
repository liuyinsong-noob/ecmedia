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
 */package com.voice.demo.tools;


import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;

/**
 * 
 * @ClassName: CCPDrawableUtils 
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-4
 *
 */
public class CCPDrawableUtils {

	/**
	 * 
	 * @param context
	 * @param id
	 * @return
	 */
	public static Drawable getDrawables(Context context , int id) {
		Drawable drawable = getResources(context).getDrawable(id);
		drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());
		
		return drawable;
	}
	
	/**
	 * 
	 * @Title: getResource 
	 * @Description: TODO 
	 * @param context
	 * @return Resources 
	 */
	public static Resources getResources(Context context) {
		return context.getResources();
	}
}
