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
public class Expert extends Response {

	/**
	 * liuyanjun for xh show
	 */
	private static final long serialVersionUID = 1607225125753254406L;
	
	private String id;
	private String categoryId;
	private String name;
	private String grade;
	private String detail;
	private String pdetail;
	
	//只作为临时头像的传递；
	private int head_icon_id;
	
	public int getHead_icon_id() {
		return head_icon_id;
	}
	public void setHead_icon_id(int head_icon_id) {
		this.head_icon_id = head_icon_id;
	}
	public String getId() {
		return id;
	}
	public void setId(String id) {
		this.id = id;
	}
	public String getCategoryId() {
		return categoryId;
	}
	public void setCategoryId(String categoryId) {
		this.categoryId = categoryId;
	}
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public String getGrade() {
		return grade;
	}
	public void setGrade(String grade) {
		this.grade = grade;
	}
	public String getDetail() {
		return detail;
	}
	public void setDetail(String detail) {
		this.detail = detail;
	}
	public String getPdetail() {
		return pdetail;
	}
	public void setPdetail(String pdetail) {
		this.pdetail = pdetail;
	}
	
	
}
