package com.voice.demo.views;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.LinearLayout;

/**
 * 
 * @ClassName: CCPLayoutListenerView.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-29
 * @version 3.6
 */
public class CCPLayoutListenerView extends LinearLayout {

	private OnCCPViewLayoutListener mOnLayoutListener;
	private OnCCPViewSizeChangedListener mOnSizeChangedListener;
	/**
	 * @param context
	 */
	public CCPLayoutListenerView(Context context) {
		super(context);
	}

	/**
	 * @param context
	 * @param attrs
	 */
	public CCPLayoutListenerView(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		super.onLayout(changed, l, t, r, b);
		if(this.mOnLayoutListener != null ) {
			this.mOnLayoutListener.onLayout();
		}
	}
	
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
		if(this.mOnSizeChangedListener != null ) {
			this.mOnSizeChangedListener.onSizeChanged(w, h, oldw, oldh);
		}
	}
	
	public void setOnLayoutListener(OnCCPViewLayoutListener onLayoutListener) {
		this.mOnLayoutListener = onLayoutListener;
	}

	public void setOnSizeChangedListener(
			OnCCPViewSizeChangedListener onSizeChangedListener) {
		this.mOnSizeChangedListener = onSizeChangedListener;
	}


	public interface OnCCPViewLayoutListener {
		void onLayout();
	}
	
	public interface OnCCPViewSizeChangedListener {
		void onSizeChanged(int w, int h, int oldw, int oldh) ;
	}
}
