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
package com.voice.demo.group.baseui;

import com.voice.demo.tools.CCPUtil;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

/**
 * <p>Title: AppGirdApapter</p>
 * <p>Description: </p>
 * <p>Company: http://www.cloopen.com/</p>
 * @author  Jorstin Chan
 * @version 3.6
 * @date 2013-12-26
 */
public class AppGirdApapter extends BaseAdapter {

	private int width;
	private int height;
	
	public AppGirdApapter(Context context) {
		
		width = CCPUtil.getMetricsDensity(context, 60.0F);
		height = CCPUtil.getMetricsDensity(context, 53.299999F);
	}
	@Override
	public int getCount() {
		return 0;
	}

	@Override
	public Object getItem(int position) {
		return null;
	}

	@Override
	public long getItemId(int position) {
		return 0;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		return null;
	}

}
