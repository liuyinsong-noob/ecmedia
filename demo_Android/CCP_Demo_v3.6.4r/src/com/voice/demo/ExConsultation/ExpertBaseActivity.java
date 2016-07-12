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
package com.voice.demo.ExConsultation;

import java.io.Serializable;
import java.util.ArrayList;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Message;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.ExConsultation.model.Category;
import com.voice.demo.ExConsultation.model.Expert;
import com.voice.demo.ExConsultation.model.ServiceNum;
import com.voice.demo.group.GroupBaseActivity;
import com.voice.demo.outboundmarketing.RestHelper.ERequestState;
import com.voice.demo.tools.net.ITask;
import com.voice.demo.ui.CCPHelper;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class ExpertBaseActivity extends GroupBaseActivity implements ExpertManagerHelper.onExpertManagerHelpListener{

	@Override
	public void onGetExpertClassic(ERequestState reason,
			ArrayList<Expert> xExperts) {
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("xExperts", xExperts);
	    msg.obj = b;
	    msg.arg1 = ExpertManagerHelper.KEY_EXPERT_LIST;
	    sendHandleMessage(msg);
	}

	@Override
	public void onGetClientGategory(ERequestState reason,
			ArrayList<Category> xcCategories) {
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("xcCategories", xcCategories);
	    msg.obj = b;
	    msg.arg1 = ExpertManagerHelper.KEY_CATEGORY_LIST;
	    sendHandleMessage(msg);
	}

	@Override
	public void onActionLockExpert(ERequestState reason) {
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    msg.obj = b;
	    msg.arg1 = ExpertManagerHelper.KEY_LOCK_EXPERT;
	    sendHandleMessage(msg);
	}

	@Override
	public void onGet400ServerPort(ERequestState reason, ServiceNum xConfig) {
		Message msg = getHandleMessage();
	    Bundle b = new Bundle();
	    b.putSerializable("ERequestState", reason);
	    b.putSerializable("xConfig", xConfig);
	    msg.obj = b;
	    msg.arg1 = ExpertManagerHelper.KEY_SERVICE_NUM;
	    sendHandleMessage(msg);
	}
	
	
	@SuppressWarnings("unchecked")
	@Override
	protected void doHandleExpertCallback(Message msg) {
		super.doHandleExpertCallback(msg);
		Bundle b = (Bundle) msg.obj;
		int what = msg.arg1;
		ERequestState reason = (ERequestState) b.getSerializable("ERequestState");
		Log4Util.i(CCPHelper.DEMO_TAG, "What: " + what);
		switch (what) {
			case ExpertManagerHelper.KEY_EXPERT_LIST:
				ArrayList<Expert> xExperts = (ArrayList<Expert>) b.getSerializable("xExperts");
				handleGetExpertClassic(reason, xExperts);
				break;
			case ExpertManagerHelper.KEY_CATEGORY_LIST:
				ArrayList<Category> xcCategories = (ArrayList<Category>) b.getSerializable("xcCategories");
				handleGetClientGategory(reason, xcCategories);
				
				break;
			case ExpertManagerHelper.KEY_LOCK_EXPERT:
				handleActionLockExpert(reason);
				break;
			case ExpertManagerHelper.KEY_SERVICE_NUM:
				ServiceNum serviceNum = (ServiceNum) b.getSerializable("xConfig");
				handleGet400ServerPort(reason, serviceNum);
				break;
				
			default:
				
				break;
		}
	}
	
	
	
	
	protected void handleGetExpertClassic(ERequestState reason,
			ArrayList<Expert> xExperts) {
		
	}
	
	protected void handleGetClientGategory(ERequestState reason,
			ArrayList<Category> xcCategories) {
		
	}
	
	protected void handleActionLockExpert(ERequestState reason) {
		
	}
	
	protected void handleGet400ServerPort(ERequestState reason, ServiceNum serviceNum) {
		
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.ui.CCPBaseActivity#onDestroy()
	 */
	@Override
	protected void onDestroy() {
		ExpertManagerHelper.getInstance().setOnExpertManagerHelpListener(null);
		super.onDestroy();
	}
	
	protected final void startAction(String action) {
		startAction(action, null, null);
	}

	protected final void startAction(String action, Uri uri) {
		Intent intent = new Intent(action, uri);
		startActivity(intent);
	}

	protected final void startAction(String action, String key, Serializable serializable) {
		Intent intent = new Intent(action);
		if (key != null && key.length() > 0 && serializable != null) {
			intent.putExtra(key, serializable);
		}
		startActivity(intent);
	}
	
	@Override
	public void addTask(ITask iTask) {
		ExpertManagerHelper.getInstance().setOnExpertManagerHelpListener(this);
		super.addTask(iTask);
	}
	
}
