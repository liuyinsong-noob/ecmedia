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
package com.voice.demo.ExConsultation.model;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ExpertList extends Response {

	/**
	 * 
	 */
	private static final long serialVersionUID = -8246344655394824068L;

	/**
		 * 
		 */
	public ArrayList<Expert> xExperts = new ArrayList<Expert>() ;

}
