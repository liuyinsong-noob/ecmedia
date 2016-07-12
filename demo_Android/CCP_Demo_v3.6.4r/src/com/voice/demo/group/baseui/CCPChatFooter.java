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
 */package com.voice.demo.group.baseui;


import java.io.File;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.group.utils.RecordPopupWindow;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.ui.CCPHelper;

import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/**
 * <p>Title: CCPChatFooter</p>
 * <p>Description: </p>
 * <p>Company: http://www.cloopen.com/</p>
 * @author  Jorstin Chan
 * @version 3.6
 * @date 2013-12-26
 */
public class CCPChatFooter extends LinearLayout implements View.OnTouchListener
															, View.OnClickListener
															, EmojiGrid.OnEmojiItemClickListener{
	
	// cancel recording sliding distance field.
	private static final int CANCLE_DANSTANCE 										= -60;
	
	private static final int ampValue[] = {
		0,20,30,45,60,85,100
	};
	private static final int ampIcon[] = {
		R.drawable.voice_interphone01,
		R.drawable.voice_interphone02,
		R.drawable.voice_interphone03,
		R.drawable.voice_interphone04,
		R.drawable.voice_interphone05,
		R.drawable.voice_interphone06,
	};

	private LayoutInflater mInflater;
	
	private LinearLayout mChatPanel;
	
	private LinearLayout mToolsStub;
	
	private AppPanel mAppPanel;
	
	private RecordPopupWindow popupWindow = null;
	private InputMethodManager mInputMethodManager;
	
	private TextView rVoiceCancleText;
	private ImageView ampImage;
	
	
	private CCPEditText mImEditText;
	private Button mVoiceReocrding;
	private Button mEmoji;
	private Button mIMsend;
	
	private View mLayoutRoot;
	private View mVoiceShortLy;
	private View mVoiceLoading;
	private View mVoiceRecRy;
	private View mCancleIcon;
	
	private ChatFooterHandle mHandle = new ChatFooterHandle();
	
	private Context mContext;
	
	private OnChattingLinstener mRecordingLinstener;
	
	private int mode;
	
	private float mTouchStartY = 0;
	private float mDistance = 0;
	
	private boolean isCancle = false;
	private boolean doReocrdAction = false;
	
	public CCPChatFooter(Context context) {
		this(context , null);
	}

	public CCPChatFooter(Context context, AttributeSet attrs) {
		this(context, attrs , 0);
	}
	
	public CCPChatFooter(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs);
		this.mContext = context;
		mInputMethodManager = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
		mInflater = LayoutInflater.from(getContext());
		mHandle = new ChatFooterHandle();
		
		initChatFooter();
	}
	
	@Override
	public boolean isInEditMode() {
		return super.isInEditMode();
	}
	
	/**
	 * 
	 */
	private void initChatFooter() {
		
		mLayoutRoot = inflate(mContext, R.layout.ccp_chatting_footer, this);
		
		mChatPanel = (LinearLayout) mLayoutRoot.findViewById(R.id.chatting_bottom_panel);
		
		mImEditText = (CCPEditText) mLayoutRoot.findViewById(R.id.im_content_et);
		
		/**
		 * modiy by version 3.4.1.1
		 */
		mImEditText.setOnTouchListener(new OnTouchListener() {
			
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				setMode(1);
				return false;
			}
		});
		
		
		mIMsend = (Button) mLayoutRoot.findViewById(R.id.im_send_btn);
		mIMsend.setOnClickListener(this);
		
		initToolsBar();
		
	}
	
	
	/**
	 * inti chatting tools
	 */
	private void initToolsBar() {
		mToolsStub = (LinearLayout) mLayoutRoot.findViewById(R.id.tools_stub);
		
		mLayoutRoot.findViewById(R.id.video_sent).setOnClickListener(this);
		mLayoutRoot.findViewById(R.id.btn_file).setOnClickListener(this);
		mEmoji = (Button) mLayoutRoot.findViewById(R.id.btn_emoji);
		mEmoji.setOnClickListener(this);
		// When a finger pressing time to start recording audio data 
		// need to change the position of the background, the opposite is 
		// the original background
		
		mVoiceReocrding = (Button) mLayoutRoot.findViewById(R.id.btn_voice);
		mVoiceReocrding.setOnTouchListener(this);
		
		mAppPanel = (AppPanel) findViewById(R.id.chatting_app_panel);
		mAppPanel.setOnEmojiItemClickListener(this);
		setMode(2 , false);
	}

	// display dialog recordings
	public void showVoiceDialog(int height) {
		int heightDensity = Math.round(180 * getResources().getDisplayMetrics().densityDpi / 160.0F);
		int density = CCPUtil.getMetricsDensity(getContext() , 50.0F);
		if(popupWindow == null ) {
			View view = mInflater.inflate(R.layout.voice_rec_dialog, null);
			popupWindow = new RecordPopupWindow(view, WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.WRAP_CONTENT);
			ampImage = ((ImageView)popupWindow.getContentView().findViewById(R.id.dialog_img));
			mCancleIcon = ((ImageView)popupWindow.getContentView().findViewById(R.id.voice_rcd_cancle_icon));
	        rVoiceCancleText = ((TextView)this.popupWindow.getContentView().findViewById(R.id.voice_rcd_cancel));
	        mVoiceLoading = this.popupWindow.getContentView().findViewById(R.id.voice_rcd_hint_loading);
	        mVoiceRecRy = this.popupWindow.getContentView().findViewById(R.id.voice_rcd_rl);
	        mVoiceShortLy = this.popupWindow.getContentView().findViewById(R.id.voice_rcd_tooshort);
		}
		mVoiceLoading.setVisibility(View.VISIBLE);
		mVoiceShortLy.setVisibility(View.GONE);
		mVoiceRecRy.setVisibility(View.GONE);
		ampImage.setVisibility(View.VISIBLE);
		ampImage.setBackgroundResource(ampIcon[0]);
		mCancleIcon.setVisibility(View.GONE);
		popupWindow.showAtLocation(this, Gravity.CENTER_HORIZONTAL|Gravity.TOP, 0, density +(height - heightDensity) / 2);
	}
	
	public synchronized void removePopuWindow() {
	    if (popupWindow != null)  {
	    	popupWindow.dismiss();
	    	mVoiceRecRy.setVisibility(View.VISIBLE);
	    	ampImage.setVisibility(View.VISIBLE);
	    	mCancleIcon.setVisibility(View.GONE);
	    	mVoiceLoading.setVisibility(View.GONE);
	    	mVoiceShortLy.setVisibility(View.GONE);
	    	rVoiceCancleText.setText(R.string.voice_cancel_rcd);
	    }
	    mVoiceReocrding.setBackgroundResource(R.drawable.voice_icon);
		mHandle.postDelayed(new Runnable() {
			
			@Override
			public void run() {
				mVoiceReocrding.setEnabled(true);
				mVoiceReocrding.setOnTouchListener(CCPChatFooter.this);
				doReocrdAction = false;
				currentTimeMillis = System.currentTimeMillis();
				Log4Util.d(CCPHelper.DEMO_TAG, "CCPChatFooter remove recording window , set enable true" );
			}
		}, 100);
	  }
	
	public synchronized void tooShortPopuWindow() {
		Log4Util.d(CCPHelper.DEMO_TAG, "CCPChatFooter voice to short , then set enable false" );
		mVoiceReocrding.setEnabled(false);
		if (popupWindow != null) {
			mVoiceShortLy.setVisibility(View.VISIBLE);
			mVoiceLoading.setVisibility(View.GONE);
			mVoiceRecRy.setVisibility(View.GONE);
			popupWindow.update();
		}
		if (mHandle != null) {
			mHandle.removeMessages(CCPHelper.WHAT_ON_DIMISS_DIALOG);
			mHandle.sendEmptyMessageDelayed(
					CCPHelper.WHAT_ON_DIMISS_DIALOG, 500L);
		}
	}
	
	/**************************** voice record   ******************************************/
	// recode 
	public void displayAmplitude(double amplitude) {
		if(mVoiceLoading == null ) {
			return;
		}
		if(mVoiceLoading.getVisibility() == View.VISIBLE) {
			
			// If you are in when being loaded, then send to start recording
			if(mRecordingLinstener != null) {
				mRecordingLinstener.onRecordStart();
			}
			
		}
		mVoiceRecRy.setVisibility(View.VISIBLE);
		mVoiceLoading.setVisibility(View.GONE);
		mVoiceShortLy.setVisibility(View.GONE);
		
		for (int i = 0; i < ampValue.length; i++) {
			if(amplitude >= ampValue[i] && amplitude < ampValue[i+1]) {
				ampImage.setBackgroundResource(ampIcon[i]);
				return ;
			}
				
			continue;
			
		}
	}
	
	public void onDistory() {
		if(mAppPanel != null ) {
			mAppPanel.releaseAppPanel();
			mAppPanel = null;
		}
		currentTimeMillis = 0;
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.video_sent:
			if(mRecordingLinstener != null) {
				mRecordingLinstener.onSelectVideo();
			}
			 
			break;
		case R.id.btn_file:
			
			if(mRecordingLinstener != null) {
				mRecordingLinstener.onSelectFile();
			}
			break;

		case R.id.btn_emoji:
			int mode = mAppPanel.isPanelVisible()? 2: 3;
			setMode(mode , false);
			break;
			
		case R.id.im_send_btn:
			
			if(mRecordingLinstener != null) {
				String text = mImEditText.getText().toString();
				mRecordingLinstener.onSendTextMesage(text);
				mImEditText.setText("");
			}
		default:
			break;
		}
	}
	
	
	long currentTimeMillis = 0;
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		
		if(getAvailaleSize() < 10) {
			Log4Util.i(CCPHelper.DEMO_TAG, "sdcard no memory ");
			Toast.makeText(mContext, R.string.media_no_memory, Toast.LENGTH_LONG).show();
			return false;
		}
		long time = System.currentTimeMillis() - currentTimeMillis;
		if(time <= 300) {
			Log4Util.i(CCPHelper.DEMO_TAG, "Invalid click ");
			currentTimeMillis = System.currentTimeMillis();
			return false;
		}

		if(!CCPUtil.isExistExternalStore()) {
			Toast.makeText(mContext, R.string.media_ejected, Toast.LENGTH_LONG).show();
			return false;
		}
		
		int[] location = new int[2];  
        v.getLocationOnScreen(location);  
        mTouchStartY = location[1];  
        switch (event.getAction()) {
		case  MotionEvent.ACTION_DOWN:
			doReocrdAction = true;
			Log4Util.d(CCPHelper.DEMO_TAG, "CCPChatFooter voice recording action down");
			if(mRecordingLinstener != null) {
				setCancle(false);
				mRecordingLinstener.onRecordInit();
			}
			
			mVoiceReocrding.setBackgroundResource(R.drawable.voice_icon_on);
			break;
			
		case MotionEvent.ACTION_MOVE:
			mDistance = event.getRawY() - mTouchStartY;
			if(mDistance < CANCLE_DANSTANCE){
				//cancle send voice ...
				if(rVoiceCancleText != null) {
					rVoiceCancleText.setText(R.string.voice_cancel_rcd_release);
					mCancleIcon.setVisibility(View.VISIBLE);
					ampImage.setVisibility(View.GONE);
				}
				isCancle = true;
			} else {
				rVoiceCancleText.setText(R.string.voice_cancel_rcd);
				mCancleIcon.setVisibility(View.GONE);
				ampImage.setVisibility(View.VISIBLE);
				isCancle = false;
			}
			
			break;
		case MotionEvent.ACTION_UP:
			if(doReocrdAction) {
				mVoiceReocrding.setEnabled(false);
				mVoiceReocrding.setOnTouchListener(null);
				Log4Util.d(CCPHelper.DEMO_TAG, "CCPChatFooter voice recording action up , then set enabel false");
				if(mRecordingLinstener != null) {
					
					if(isCancle) {
						mRecordingLinstener.onRecordCancle();
					} else {
						mRecordingLinstener.onRecordOver();
					}
				}
			}
			break;
		}
		return false;
	}
	
	
	public void onResume() {
		
	}
		
	public void onPause() {
	
	}
	
	
	public boolean isVoiceRecordCancle() {
		return isCancle;
	}

	public void setCancle(boolean isCancle) {
		this.isCancle = isCancle;
	}


	final class ChatFooterHandle extends Handler {
		
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			
			switch (msg.what) {
			case CCPHelper.WHAT_ON_DIMISS_DIALOG:

				removePopuWindow();
				break;
			case CCPHelper.WHAT_ON_AMPLITUDE:
				if (msg.obj instanceof Bundle) {
					Bundle b = (Bundle) msg.obj;
					double amplitude = b.getDouble(Device.VOICE_AMPLITUDE);
					displayAmplitude(amplitude);
				}
				
				break;
				
			default:
				break;
			}
		}
	}
	
	/**
	 * @param mode
	 */
	public void setMode(int mode) {
		setMode(mode, true);
	}
	
	/**
	 * @param mode
	 * @param b
	 */
	public void setMode(int mode , boolean input) {
		
		this.mode = mode;
		switch (mode) {
		case 1:
			// Does not display the tools bar
			mImEditText.requestFocus();
			resetChatFooter(false);
			break;
		case 2:
			
			// display the tools bar, But do not show expression panel
			resetChatFooter(true , false);
			break;
			
		case 3:
			// Display tools expression and panel
			resetChatFooter(true , true);
			break;
		default:
			setVisibility(View.VISIBLE);
			break;
		}
		
		if(input) {
			mInputMethodManager.showSoftInput(mImEditText, 0);
		} else {
			mInputMethodManager.hideSoftInputFromWindow(mImEditText.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
		}
	}
	
	/**
	 * @param isTools
	 */
	private void resetChatFooter(boolean isTools) {
		resetChatFooter(isTools, false);
	}
	
	/**
	 * Reset chat footer tool panel
	 * @param isEmoji Whether to display the expression panel
	 */
	private void resetChatFooter(boolean isTools , boolean isEmoji) {
		mChatPanel.setVisibility(View.VISIBLE);
		if(!isTools) {
			mToolsStub.setVisibility(View.GONE);
			mAppPanel.setPanelGone();
			return;
		}
		mToolsStub.setVisibility(View.VISIBLE);
		if(isEmoji) {
			mEmoji.setBackgroundResource(R.drawable.facial_expression_icon_on);
			mAppPanel.swicthToPanel(AppPanel.APP_PANEL_NAME_DEFAULT);
			mAppPanel.setVisibility(View.VISIBLE);
		} else {
			mEmoji.setBackgroundResource(R.drawable.facial_expression_icon);
			mAppPanel.setPanelGone();
		}
	}
	
	/**
	 * @return
	 */
	public int getMode() {
		return mode;
	}
	
	
	public void setOnChattingLinstener(OnChattingLinstener l) {
		mRecordingLinstener = l;
	}
	
	/**
	* <p>Title: OnRecordingLinstener</p>
	* <p>Description: </p>
	* <p>Company: http://www.cloopen.com/</p>
	* @author  Jorstin Chan
	* @version 3.6
	* @date 2013-12-26
	 */
	public interface OnChattingLinstener {
		
		void onRecordInit();
		void onRecordStart();
		void onRecordCancle();
		void onRecordOver();
		
		void onSendTextMesage(String text);
		
		void onSelectFile();
		void onSelectVideo();
	}

	@Override
	public void onEmojiItemClick(int emojiid, String emojiName) {
		mImEditText.setEmojiText(emojiName);
		
	}

	@Override
	public void onEmojiDelClick() {
		mImEditText.getInputConnection().sendKeyEvent(
				new KeyEvent(MotionEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL));
		mImEditText.getInputConnection().sendKeyEvent(
				new KeyEvent(MotionEvent.ACTION_UP, KeyEvent.KEYCODE_DEL));
	}
	
	public long getAvailaleSize(){

		File path = Environment.getExternalStorageDirectory(); //取得sdcard文件路径
		StatFs stat = new StatFs(path.getPath()); 
		long blockSize = stat.getBlockSize(); 
		long availableBlocks = stat.getAvailableBlocks();
//		return availableBlocks * blockSize; 
		//(availableBlocks * blockSize)/1024      KIB 单位
		return (availableBlocks * blockSize)/1024 /1024;//  MIB单位
	}
}
