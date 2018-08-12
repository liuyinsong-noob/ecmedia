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
package com.voice.demo.ExConsultation.model;


import com.hisun.phone.core.voice.model.Response;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class Category extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = 4617198056010032875L;
	
	private String category_id;
	private String category_name;
	private String category_detail;
	private String category_postition;
	
	public String getCategory_id() {
		return category_id;
	}
	public void setCategory_id(String category_id) {
		this.category_id = category_id;
	}
	public String getCategory_name() {
		return category_name;
	}
	public void setCategory_name(String category_name) {
		this.category_name = category_name;
	}
	public String getCategory_detail() {
		return category_detail;
	}
	public void setCategory_detail(String category_detail) {
		this.category_detail = category_detail;
	}
	public String getCategory_postition() {
		return category_postition;
	}
	public void setCategory_postition(String category_postition) {
		this.category_postition = category_postition;
	}

}
