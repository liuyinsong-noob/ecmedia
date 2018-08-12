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
package com.voice.demo.views;

import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;

import android.app.Activity;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * 
* <p>Title: CCPTitleViewBase.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-10-31
* @version 3.5
 */
public class CCPTitleViewBase {

	private CCPBaseActivity mCCPActivity ;
	private TextView mAppTitle ;
	private TextView mSubTitle ;
	
	private ImageView mTitlePhone;
	private ImageView mTitlePlaceholder;
	private ImageView mTitleMute;
	
	private CCPImageButton mCCPBackImageButton;
	private CCPImageButton mCCPActionImageButton;
	
	private LinearLayout mTitleViewRoot;
	public CCPTitleViewBase(Activity activity) {
		
		mCCPActivity = (CCPBaseActivity) activity;
		
		mCCPActionImageButton = (CCPImageButton) activity.findViewById(R.id.title_btn1);
		mCCPBackImageButton = (CCPImageButton) activity.findViewById(R.id.title_btn4);
		
		mTitleViewRoot = (LinearLayout) activity.findViewById(R.id.nav_title);
		
		mTitlePhone = (ImageView) activity.findViewById(R.id.title_phone);
		mTitlePlaceholder = (ImageView) activity.findViewById(R.id.title_phone_placeholder);
		mTitleMute = (ImageView) activity.findViewById(R.id.title_mute);
		
		mAppTitle = (TextView) activity.findViewById(R.id.title);
		mSubTitle = (TextView) activity.findViewById(R.id.sub_title);
	}
	
	public CCPImageButton setCCPActionImageButton(Object object , View.OnClickListener l) {
		
		if(mCCPActionImageButton == null) {
			return null;
		}
		
		mCCPActionImageButton.setVisibility(View.VISIBLE);
		mCCPActionImageButton.setOnClickListener(l);
		if(object instanceof String) {
			mCCPActionImageButton.setText((String)object);
			
		} else if (object instanceof Drawable) {
			mCCPActionImageButton.setImageDrawable((Drawable)object);
		} else {
			mCCPActionImageButton.setImageResource(((Integer)object).intValue());
		}
		
		if(mCCPBackImageButton != null && mCCPBackImageButton.getVisibility() != View.VISIBLE) {
			mCCPBackImageButton.setVisibility(View.INVISIBLE);
		}
		
		return mCCPActionImageButton;
	}
	
	
	public CCPImageButton setCCPBackImageButton(Object object , View.OnClickListener l) {
		
		if(mCCPBackImageButton == null) {
			return null;
		}
		
		mCCPBackImageButton.setVisibility(View.VISIBLE);
		mCCPBackImageButton.setOnClickListener(l);
		if(object instanceof String) {
			mCCPBackImageButton.setText((String)object);
			
		} else if (object instanceof Drawable) {
			mCCPBackImageButton.setImageDrawable((Drawable)object);
		} else {
			mCCPBackImageButton.setImageResource(((Integer)object).intValue());
		}
		
		if(mCCPActionImageButton != null && mCCPActionImageButton.getVisibility() != View.VISIBLE) {
			mCCPActionImageButton.setVisibility(View.INVISIBLE);
		}
		
		return mCCPBackImageButton;
	}
	
	public View getCCPTitleLinearLayout() {
		return mTitleViewRoot;
	}
	
	public CCPImageButton getCCPImageButtonAction() {
		return mCCPActionImageButton;
	}
	
	public void setCCPTitleBackground(int resId) {
		if(mTitleViewRoot != null)
			mTitleViewRoot.setBackgroundResource(resId);
	}
	
	
	public CCPImageButton setCCPActionText(int resId , View.OnClickListener l) {
		
		String text = mCCPActivity.getResources().getString(resId);
		return setCCPActionImageButton(text, l);
	}
	
	public CCPImageButton setCCPBackText(int resId , View.OnClickListener l) {
		
		String text = mCCPActivity.getResources().getString(resId);
		return setCCPBackImageButton(text, l);
	}
	
	public void setCCPActionEnable(boolean enabled) {
		
		if(mCCPActionImageButton != null) {
			mCCPActionImageButton.setEnabled(enabled);
		}
	}
	
	public void setAppTitleOnClickListener(View.OnClickListener l) {
		if(mAppTitle != null) {
			mAppTitle.setOnClickListener(l);
		}
	}
	
	
	public void setTitlePhoneVisiable(int visibility) {
		
		if(visibility == View.VISIBLE) {
			mTitlePhone.setVisibility(visibility);
			mTitlePlaceholder.setVisibility(View.INVISIBLE);
		} else {
			
			mTitlePhone.setVisibility(visibility);
			mTitlePlaceholder.setVisibility(visibility);
		}
	}
	
	public void setTitleMuteVisiable(int visibility) {
		
		if(visibility == View.VISIBLE) {
			mTitleMute.setVisibility(visibility);
			mTitlePlaceholder.setVisibility(View.INVISIBLE);
		} else {
			
			mTitleMute.setVisibility(visibility);
			mTitlePlaceholder.setVisibility(visibility);
		}
	}
	
	
	public void setTitleMuteImageResource(int resId) {
		mTitleMute.setImageResource(resId);
		
	}
	
	public void setCCPTitleViewVisibility(int visibility) {
		if(mAppTitle != null) {
			mAppTitle.setVisibility(visibility);
		}
	}
	
	public void setCCPTitleViewText(CharSequence text) {
		if(mAppTitle != null) {
			mAppTitle.setText(text);
		}
		
		setCCPTitleTextSize(mCCPActivity.getResources().getDimensionPixelSize(R.dimen.title_large_text_size));
	}
	
	
	/**
	 * 
	 * @Title: setTextSize 
	 * @Description: TODO 
	 * @param @param size 
	 * @return void 
	 * @throws
	 */
	public final void setCCPTitleTextSize(float size) {
		setCCPTitleTextSize(TypedValue.COMPLEX_UNIT_PX,size);
	}
	
	/**
	 * 
	 * @Title: setTextSize 
	 * @Description: TODO 
	 * @param @param unit
	 * @param @param size 
	 * @return void 
	 * @throws
	 */
	public final void setCCPTitleTextSize(int unit , float size) {
		mAppTitle.setTextSize(unit,size);
	}
	
	
	public void setCCPActionVisibility(int visibility) {
		
		if(mCCPActionImageButton != null) {
			mCCPActionImageButton.setVisibility(visibility);
		}
	}
	
	public void setCCPBackVisibility(int visibility) {
		
		if(mCCPBackImageButton != null) {
			mCCPBackImageButton.setVisibility(visibility);
		}
	}
	
	public void setCCPSubTitleViewText(CharSequence text) {
		if(mSubTitle != null) {
			
			if(TextUtils.isEmpty(text)) {
				mSubTitle.setVisibility(View.GONE);
			} else {
				mSubTitle.setVisibility(View.VISIBLE);
				mAppTitle.setText(text);
			}
			
		}
	}
}
