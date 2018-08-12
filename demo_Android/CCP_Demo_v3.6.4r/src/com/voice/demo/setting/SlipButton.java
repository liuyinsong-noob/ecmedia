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

package com.voice.demo.setting;

import com.voice.demo.R;


import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;


/**
 * 
 *  左右滑动块
 *
 */
public class SlipButton extends View implements OnTouchListener{
	private String strName;
	private boolean enabled = true;
	public boolean flag = false;//设置初始化状态 
	public boolean NowChoose = false;//记录当前按钮是否打开,true为打开,flase为关闭
	private boolean OnSlip = false;//记录用户是否在滑动的变量
	public float DownX=0f,NowX=0f;//按下时的x,当前的x,NowX>100时为ON背景,反之为OFF背景
	private Rect Btn_On,Btn_Off;//打开和关闭状态下,游标的Rect

	private boolean isChgLsnOn = false;
	private OnChangedListener ChgLsn;
	private Bitmap bg_on,bg_off,slip_btn;


	public SlipButton(Context context) {
		super(context);
		init();
	}

	public SlipButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	public void setChecked(boolean fl){
		if(fl){
			flag = true; NowChoose = true; NowX = 80;
		}else{
			flag = false; NowChoose = false; NowX = 0;
		}
		refreshDrawableState();
		invalidate();//重画控件
	}

	public boolean isChecked(){
		return NowChoose;
	}

	public void setEnabled(boolean b){
		if(b){
			enabled = true;
		}else{
			enabled = false;
		}
	}

	private void init(){//初始化
		//载入图片资源
		bg_on = BitmapFactory.decodeResource(getResources(), R.drawable.open_icon_bg);
		bg_off = BitmapFactory.decodeResource(getResources(), R.drawable.close_icon_bg);
		slip_btn = BitmapFactory.decodeResource(getResources(), R.drawable.open_close_icon);
		//获得需要的Rect数据
		Btn_On = new Rect(0,0,slip_btn.getWidth(),slip_btn.getHeight());
		Btn_Off = new Rect(
				bg_off.getWidth()-slip_btn.getWidth(),
				0,
				bg_off.getWidth(),
				slip_btn.getHeight());
		setOnTouchListener(this);//设置监听器,也可以直接复写OnTouchEvent
	}

	@Override
	protected void onDraw(Canvas canvas) {//绘图函数
		super.onDraw(canvas);
		Matrix matrix = new Matrix();
		Paint paint = new Paint();
		float x;
		{  
			if (flag) {NowX = 80;flag = false;
			}//bg_on.getWidth()=71
			if(NowX<(bg_on.getWidth()/2)){//滑动到前半段与后半段的背景不同,在此做判断// 这个地方是有问题的；
				canvas.drawBitmap(bg_off,matrix, paint);//画出关闭时的背景
			}
			else {
				canvas.drawBitmap(bg_on,matrix, paint);//画出打开时的背景
			}
			if(OnSlip)//是否是在滑动状态,
			{   
				if(NowX >= bg_on.getWidth())//是否划出指定范围,不能让游标跑到外头,必须做这个判断
					x = bg_on.getWidth()-slip_btn.getWidth()/2;//减去游标1/2的长度...
				else
					x = NowX - slip_btn.getWidth()/2;
			}else{//非滑动状态
				if(NowChoose)//根据现在的开关状态设置画游标的位置
					x = Btn_Off.left;
				else
					x = Btn_On.left;
			}

			if(x<0)//对游标位置进行异常判断...
				x = 0;
			else if(x>bg_on.getWidth()-slip_btn.getWidth())
				x = bg_on.getWidth()-slip_btn.getWidth();
			canvas.drawBitmap(slip_btn,x, 0, paint);//画出游标.
		}
	}


	public boolean onTouch(View v, MotionEvent event) {
		if(!enabled){
			return false;
		}
		boolean LastChoose = false;
		switch(event.getAction())//根据动作来执行代码
		{
		case MotionEvent.ACTION_MOVE://滑动
			NowX = event.getX();// 滑动的起点;
			break;
		case MotionEvent.ACTION_DOWN://按下
			if(event.getX()>bg_on.getWidth()||event.getY()>bg_on.getHeight()) // 限定其滑动位置, 坐标
				return false;
			OnSlip = true;
			DownX = event.getX();
			NowX = DownX;
			break;
		case MotionEvent.ACTION_UP://松开
			OnSlip = false;
			LastChoose = NowChoose;
			//event.getX 是相对于容器的坐标;
			if(event.getX()>=(bg_on.getWidth()/2)) {
				NowChoose = true;}
			else
				NowChoose = false;
			if(isChgLsnOn&&(LastChoose!=NowChoose))//如果设置了监听器,就调用其方法..
				ChgLsn.onChanged(strName, NowChoose);
			break;
		default:
			OnSlip = false;
			LastChoose = NowChoose;
			if(NowX >=(bg_on.getWidth()/2)) {
				NowChoose = true;}
			else
				NowChoose = false;
			if(isChgLsnOn&&(LastChoose!=NowChoose))//如果设置了监听器,就调用其方法..
				ChgLsn.onChanged(strName, NowChoose);
			break;
		}
		invalidate();//重画控件
		return true;
	}

	public void SetOnChangedListener(String name, OnChangedListener l){//设置监听器,当状态修改的时候
		strName = name;
		isChgLsnOn = true;
		ChgLsn = l;
	}

	public interface OnChangedListener {
		public void onChanged(String strname , boolean checkState);
	}
}
