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
package com.voice.demo.group.baseui;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.TranslateAnimation;
import android.widget.HorizontalScrollView;

/**
 * 
 * @ClassName: CCPSmoothHorizontalScrollView.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-29
 * @version 3.6
 */
public class CCPSmoothHorizontalScrollView extends HorizontalScrollView {

	private View inner;
	private Rect rect = new Rect();
	
	private TranslateAnimation mTranslateAnimation;
	
	private Interpolator mInterpolator = new DecelerateInterpolator();
	
	private float mTouchX;
	
	/**
	 * @param context
	 */
	public CCPSmoothHorizontalScrollView(Context context) {
		this(context , null);
	}

	/**
	 * @param context
	 * @param attrs
	 */
	public CCPSmoothHorizontalScrollView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setFadingEdgeLength(0);
	}
	
	@Override
	protected void onFinishInflate() {
		if(getChildCount() > 0) {
			inner = getChildAt(0);
		}
	}

	@Override
	public boolean onTouchEvent(MotionEvent ev) {
		if(inner == null ) {
			
			return super.onTouchEvent(ev);
		} 
		
		switch (ev.getAction()) {
		case MotionEvent.ACTION_DOWN:
			mTouchX = ev.getX();
			
			break;
		case MotionEvent.ACTION_MOVE:
			float x = ev.getX();
			if(mTouchX == 0.0F) {
				mTouchX = x;
			}
			int distance = (int) (mTouchX - x) / 2 ;
			scrollBy(distance, 0);
			mTouchX = x;

			if(isSlideMove()) {
				
				if(rect.isEmpty()) {
					rect.set(inner.getLeft(), inner.getTop(), inner.getRight(), inner.getBottom());
				}
				inner.layout(inner.getLeft() - distance , inner.getTop(), inner.getRight() - distance , inner.getBottom());
				
			}
			
			break;
		case MotionEvent.ACTION_CANCEL:
		case MotionEvent.ACTION_UP:
			
			if(!rect.isEmpty()) {
				doAnimation();
			}
			
			break;

		default:
			break;
		}
		
		return super.onTouchEvent(ev);
	}
	
	
	/**
	 * 
	 */
	private void doAnimation() {
		mTranslateAnimation = new TranslateAnimation(inner.getLeft(), rect.left, 0.0F, 0.0F);
		mTranslateAnimation.setInterpolator(mInterpolator);
		mTranslateAnimation.setDuration(Math.abs((inner.getLeft()- rect.left)));
		inner.setAnimation(mTranslateAnimation);
		inner.layout(rect.left, rect.top, rect.right, rect.bottom);
		rect.setEmpty();
	}
	
	
	
	// Whether you need to move the layout
	public boolean isSlideMove() {
		int offset = inner.getMeasuredWidth() - getWidth();
		int scrollX = getScrollX();
		if (scrollX == 0 || scrollX == offset) {
			return true;
		}
		return false;
	}
	
	
	
	
	
	
}
