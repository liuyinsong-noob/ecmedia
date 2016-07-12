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
package com.voice.demo.views;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.ui.CCPHelper;

import android.app.Activity;
import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.RelativeLayout;

public class GroupChatLayout extends RelativeLayout {

	//private static final String TAG = "GroupChatLayout";

    private static Activity mGroupActivity;;
	
	public GroupChatLayout(Context context) {
		super(context);
	}

	public GroupChatLayout(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public GroupChatLayout(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}
	
	public static void setGroupChatActivity(Activity groupActivity) {
		mGroupActivity = groupActivity;
    }
	
	/**
     * Overrides the handling of the back key to move back to the 
     * previous sources or dismiss the search dialog, instead of 
     * dismissing the input method.
     */
	@Override
	public boolean dispatchKeyEventPreIme(KeyEvent event) {
		
		Log4Util.d(CCPHelper.DEMO_TAG, "dispatchKeyEventPreIme(" + event + ")");
        if (mGroupActivity != null && 
                    event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            KeyEvent.DispatcherState state = getKeyDispatcherState();
            if (state != null) {
                if (event.getAction() == KeyEvent.ACTION_DOWN
                        && event.getRepeatCount() == 0) {
                    state.startTracking(event, this);
                    return true;
                } else if (event.getAction() == KeyEvent.ACTION_UP
                        && !event.isCanceled() && state.isTracking(event)) {
                	mGroupActivity.onKeyDown(event.getKeyCode(), event);
                    return true;
                }
            }
        }

        
		return super.dispatchKeyEventPreIme(event);
	}

}
