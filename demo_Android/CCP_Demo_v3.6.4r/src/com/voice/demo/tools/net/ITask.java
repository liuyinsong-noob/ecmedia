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
package com.voice.demo.tools.net;

import java.util.HashMap;

/**
 * Network task task
 * 
 * @version Time: 2013-7-26
 */
public class ITask {

	private HashMap<String, Object> parameters = new HashMap<String, Object>();
	private int key;
	private int paramsCount;

	public ITask() {
		super();
	}

	public int getParamsCount() {
		return paramsCount;
	}

	public void setParamsCount(int paramsCount) {
		this.paramsCount = paramsCount;
	}

	public ITask(int key) {
		super();
		this.key = key;
	}

	public int getKey() {
		return key;
	}

	public void setKey(int key) {
		this.key = key;
	}

	public void setTaskParameters(String key, Object value) {
		parameters.put(key, value);
	}

	public Object getTaskParameters(String key) {
		return parameters.remove(key);
	}
	
	public boolean containsKey(String key) {
		return parameters.containsKey(key);
	}
	
}
