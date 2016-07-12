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
 */package com.voice.demo.ui.baseui;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.voice.demo.R;
import com.voice.demo.views.CCPButton;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
* <p>Title: CCPCapacityItemView.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2013</p>
* <p>Company: http://www.cloopen.com</p>
* @author Jorstin Chan
* @date 2013-12-2
* @version 3.6
 */
public class CCPCapabilityItemView extends LinearLayout {

	public HashMap<Integer, CCPButton> capacityItemCache = new HashMap<Integer, CCPButton>();
	
	/**
	 * 
	 */
	private Context mContext;
	
	/**
	 * 
	 */
	private LayoutInflater mLayoutInflater;

	/**
	 * 
	 */
	private int space;
	
	/**
	 * 
	 */
	private TextView textViewImage;
	
	/**
	 * 
	 */
	private OnCapacityItemClickListener mLinstener;

	/**
	 * 
	 */
	private LinearLayout capacitySubLayout;

	/**
	 * 
	 */
	private LinearLayout mLayout;
	
	private int mCapabilityId;
	
	public CCPCapabilityItemView(Context context) {
		super(context);
		init(context);
	}

	public CCPCapabilityItemView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(context);
	}

	public CCPCapabilityItemView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		
		init(context);
		
		
	}
	
	public void setCapabilityId(int id) {
		mCapabilityId = id;
	}

	private void init(Context context) {
		mContext = context;
		mLayoutInflater = LayoutInflater.from(context);
		
		space = Math.round(6 * getResources().getDisplayMetrics().densityDpi / 160.0F);
		
		setOrientation(LinearLayout.HORIZONTAL);
		mLayout = new LinearLayout(context);
		mLayout.setOrientation(LinearLayout.VERTICAL);
		LinearLayout.LayoutParams bottomLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT
				,LinearLayout.LayoutParams.MATCH_PARENT);
		bottomLayoutParams.gravity = Gravity.CENTER_HORIZONTAL;
		bottomLayoutParams.weight = 2;
		mLayout.setLayoutParams(bottomLayoutParams);
		
		
		
		FrameLayout capacityItem = getCapacityItem(CapacityType.Type_captain , null);
		mLayout.addView(capacityItem);
		
		addView(mLayout);
		
		capacitySubLayout = new LinearLayout(mContext);
		capacitySubLayout.setOrientation(LinearLayout.VERTICAL);
		LinearLayout.LayoutParams capacitySubLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT
				,LinearLayout.LayoutParams.MATCH_PARENT);
		capacitySubLayoutParams.gravity = Gravity.CENTER_HORIZONTAL;
		capacitySubLayoutParams.weight = 1;
		capacitySubLayoutParams.leftMargin = space;
		capacitySubLayout.setLayoutParams(capacitySubLayoutParams);
		
		addView(capacitySubLayout);
	}
	
	/**
	 * 
	 * @Title: getCapacityItem 
	 * @Description: TODO 
	 * @param @param capacityType
	 * @param @param resid
	 * @param @return 
	 * @return FrameLayout 
	 * @throws
	 */
	private FrameLayout getCapacityItem(final CapacityType capacityType , final int[] resid) {
		FrameLayout fLayout = (FrameLayout) mLayoutInflater.inflate(R.layout.ccp_capacity_item, null);
		LinearLayout.LayoutParams rfLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT
				,LinearLayout.LayoutParams.MATCH_PARENT);
		rfLayoutParams.gravity = Gravity.CENTER_HORIZONTAL;
		
		textViewImage = (TextView) fLayout.findViewById(R.id.capacity_text);
		CCPButton mCCPButton = (CCPButton) fLayout.findViewById(R.id.ccp_item_button);
		
		if(capacityType == CapacityType.Type_captain || capacityType == null) {
			rfLayoutParams.topMargin = 2*space;
			rfLayoutParams.weight = 2;
			rfLayoutParams.rightMargin = space;
			
			textViewImage.setVisibility(View.VISIBLE);
			mCCPButton.setVisibility(View.GONE);
			
			if(capacityType == null) {
				fLayout.setVisibility(View.INVISIBLE);
			}
		} else {
			rfLayoutParams.weight = 1;
			
			if (capacityType == CapacityType.Type_subClassifyL) {
				rfLayoutParams.rightMargin = space;
			} else {
				
				rfLayoutParams.leftMargin = space;
			}
			
			if(resid != null && resid.length == 2) {
				mCCPButton.setCCPButtonImageResource(resid[0]);
				mCCPButton.setCCPButtonBackground(resid[1]);
				
				mCCPButton.setOnClickListener(new OnClickListener() {
					
					@Override
					public void onClick(View v) {
						if(mLinstener != null ) {
							mLinstener.OnCapacityItemClick(mCapabilityId ,resid[0]);
						}
					}
				});
				
				capacityItemCache.put(resid[1], mCCPButton);
			} else {
				fLayout.setVisibility(View.INVISIBLE);
			}
			
			textViewImage.setVisibility(View.GONE);
			mCCPButton.setVisibility(View.VISIBLE);
			
		}
		
		
		fLayout.setLayoutParams(rfLayoutParams);
		fLayout.setBackgroundResource(R.color.black);
		fLayout.getBackground().setAlpha(55);
		
		return fLayout;
		
	}
	
	/**
	 * 
	 * @Title: getSubCapacityItem 
	 * @Description: TODO 
	 * @param @param resids
	 * @param @return 
	 * @return LinearLayout 
	 * @throws
	 */
	private LinearLayout getSubCapacityItem(List<Map<Integer, Integer>> capacityViews) {

		LinearLayout layout = null;
		LinearLayout.LayoutParams capacitySubLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT
				,LinearLayout.LayoutParams.MATCH_PARENT);
		capacitySubLayoutParams.topMargin = 2*space;
		capacitySubLayoutParams.weight = 1;
		FrameLayout capacityItem = null;
		int index = 1;
		for(Map<Integer, Integer> resids : capacityViews) {
			CapacityType capacityType = index%2 == 0?CapacityType.Type_subClassifyR :CapacityType.Type_subClassifyL;
			for (Map.Entry<Integer, Integer> entry : resids.entrySet()) {
				capacityItem = getCapacityItem(capacityType, new int[]{entry.getKey().intValue() , entry.getValue().intValue()});
			}
			if(capacityType == CapacityType.Type_subClassifyL) {
				layout = new LinearLayout(mContext);
				layout.setLayoutParams(capacitySubLayoutParams);
				
				if(capacityViews.size() > 2 && index >= 2) {
					mLayout.addView(getCapacityItem(null , null));
				}
			}
			
			layout.addView(capacityItem);
			
			if(capacityType == CapacityType.Type_subClassifyR) {
				capacitySubLayout.addView(layout);
			}
			index ++;
		}
		
		if(index %2 == 0) {
			capacityItem = getCapacityItem(CapacityType.Type_subClassifyR, null);
			layout.addView(capacityItem);
			capacitySubLayout.addView(layout);
		}
		
		return capacitySubLayout;
	}
	
	
	/**
	 * 
	 * @Title: setCapacityText 
	 * @Description: TODO 
	 * @param @param text 
	 * @return void 
	 * @throws
	 */
	public void setCapacityText(CharSequence text) {
		if(textViewImage != null) {
			textViewImage.setText(text);
		}
	}
	
	/**
	 * 
	 * @Title: setCapacityText 
	 * @Description: TODO 
	 * @param @param resid 
	 * @return void 
	 * @throws
	 */
	public void setCapacityText(int resid) {
		if(textViewImage != null) {
			textViewImage.setText(resid);
		}
	}
	
	/**
	 * 
	 * @Title: setCapacityItem 
	 * @Description: TODO 
	 * @param @param resids 
	 * @return void 
	 * @throws
	 */
	public void setCapacityItem(List<Map<Integer, Integer>> capacityViews) {
		getSubCapacityItem(capacityViews);
	}
	
	/**
	 * 
	 * @Title: setmLinstener 
	 * @Description: TODO 
	 * @param @param l 
	 * @return void 
	 * @throws
	 */
	public void setOnCapacityItemClickListener(OnCapacityItemClickListener l) {
		this.mLinstener = l;
	}


	/**
	 * 
	 * @ClassName: CapacityType 
	 * @Description: TODO
	 * @author Jorstin Chan 
	 * @date 2013-12-3
	 *
	 */
	public enum CapacityType {
		Type_captain,Type_subClassifyL ,Type_subClassifyR
	}
	
	/**
	 * 
	 * @ClassName: OnCapacityItemClickListener 
	 * @Description: Interface definition for a callback to be invoked when a view is clicked.
	 * @author Jorstin Chan 
	 * @date 2013-12-3
	 *
	 */
	public interface OnCapacityItemClickListener {
		/**
		 * Called when a view has been clicked.
		 * @param resid
		 */
		void OnCapacityItemClick(int id , int resid);
	}
	
}
