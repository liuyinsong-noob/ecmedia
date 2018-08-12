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
package com.voice.demo.outboundmarketing;


import java.io.ByteArrayInputStream;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParser;

import android.text.TextUtils;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.net.HttpManager;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.outboundmarketing.model.LandingCall;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.net.BaseRestHelper;

/**
 * Tell developer how can get sub-account and sub-password etc. by REST network
 * interface.
 * 
 * @version 1.0.0
 */
public class RestHelper extends BaseRestHelper{

	public static final String TAG = RestHelper.class.getSimpleName();
	private static RestHelper instance;
	
	public static final int KEY_LANDING_CALL = 970101;
	public static final int KEY_VERIFY_CODE = 970102;

	public static RestHelper getInstance() {
		if(instance == null ) {
			instance = new RestHelper();
		}
		
		return instance;
	}
	private onRestHelperListener mListener;

	// Private Constructs
	private RestHelper() {

	}
	
	public enum ERequestState {
		Success ,Failed
	}

	/**
	 * @param toVoip
	 * @param audioName
	 * @param appId
	 * @param diaplayNum
	 */
	public void LandingCalls(String toVoip ,String audioName ,String appId , String diaplayNum) {
		
		int requestType = KEY_LANDING_CALL;
		String formatTimestamp = VoiceUtil.formatTimestamp(System
				.currentTimeMillis());

		// request url
		StringBuffer urlBuffer = getAccountRequestURL(requestType, formatTimestamp);
		String url = urlBuffer.toString();

		// request header
		HashMap<String, String> headers = getAccountRequestHead(requestType, formatTimestamp);

		// request body
		// marketingcall.wav
		final StringBuffer requestBody = new StringBuffer("<LandingCall>\r\n");
		requestBody.append("\t<appId>").append(appId).append("</appId>\r\n");
		if(!TextUtils.isEmpty(audioName)) {
			requestBody.append("\t<mediaName>").append(audioName).append("</mediaName>\r\n");
		} else {
			requestBody.append("\t<mediaName type=\"1\">").append("ccp_marketingcall.wav").append("</mediaName>\r\n");
//			requestBody.append("<mediaTxt>").append(CCPApplication.getInstance().getResources().getString(R.string.str_outcalls_default_mediatext)).append("</mediaTxt>");
		}
		requestBody.append("\t<to>").append(toVoip).append("</to>\r\n");
		
		if(!TextUtils.isEmpty(diaplayNum)) {
			requestBody.append("\t<diaplayNum>").append(diaplayNum).append("</diaplayNum>\r\n");
		}
		requestBody.append("</LandingCall>\r\n");
		Log4Util.i(TAG, requestBody.toString());
		
		LandingCall landingCall = null;
		try {
			String xml = HttpManager.httpPost(url.toString(), headers,
					requestBody.toString());
			Log4Util.d(Device.TAG, xml + "\r\n");

			landingCall = (LandingCall) doParserResponse(requestType,new ByteArrayInputStream(xml.getBytes()));
			if(landingCall == null) {
				landingCall = new LandingCall();
			}
			landingCall.setPhoneNumber(toVoip);
			ERequestState state = landingCall.isError()? ERequestState.Failed : ERequestState.Success;
			if (mListener != null) {
				
				mListener.onLandingCAllsStatus(state ,landingCall);
			}
			
		} catch (Exception e) {
			e.printStackTrace();
			if (mListener != null) {
				mListener.onLandingCAllsStatus(ERequestState.Failed ,landingCall);
			}
		} finally {
			if (headers != null) {
				headers.clear();
				headers = null;
			}
		}
	}
	
	public void VoiceVerifyCode(String verifyCode ,String playTimes , String toVoip , String diaplayNum , String respUrl) {
		
		int keyValue = KEY_VERIFY_CODE;
		
		String formatTimestamp = VoiceUtil.formatTimestamp(System.currentTimeMillis());
		
		// request url
		StringBuffer urlBuf = getAccountRequestURL(keyValue, formatTimestamp);
		String url = urlBuf.toString();
		
		Log4Util.w(Device.TAG, "url: " + url + "\r\n");
		
		checkHttpUrl(url);
		
		// request header
		HashMap<String, String> headers = getAccountRequestHead(keyValue, formatTimestamp);

		final StringBuffer requestBody = new StringBuffer("<VoiceVerify>\r\n");
		requestBody.append("\t<appId>").append(CCPConfig.App_ID).append("</appId>\r\n");
		requestBody.append("\t<verifyCode>").append(verifyCode).append("</verifyCode>\r\n");
		requestBody.append("\t<playTimes>").append(playTimes).append("</playTimes>\r\n");
		requestBody.append("\t<to>").append(toVoip).append("</to>\r\n");
		if(!TextUtils.isEmpty(diaplayNum)) {
			requestBody.append("\t<diaplayNum>").append(diaplayNum).append("</diaplayNum>\r\n");
		}
		if(!TextUtils.isEmpty(respUrl)) {
			requestBody.append("\t<respUrl>").append(respUrl).append("</respUrl>\r\n");
		}
		requestBody.append("</VoiceVerify>\r\n");
		Log4Util.i(TAG, requestBody.toString());

		try {
			String xml = HttpManager.httpPost(url.toString(), headers,
					requestBody.toString());
			Log4Util.d(Device.TAG, xml + "\r\n");

			Response response = doParserResponse(KEY_VERIFY_CODE,new ByteArrayInputStream(xml.getBytes()));
			
			if (response != null) {
				if (response.isError()) {
					if (mListener != null) {
						mListener.onVoiceCode(ERequestState.Failed);
					}
				} else {
					if (mListener != null) {
						mListener.onVoiceCode(ERequestState.Success);
					}
				}
			} else {
				if (mListener != null) {
					mListener.onVoiceCode(ERequestState.Failed);
				}
			}
			
		} catch (Exception e) {
			e.printStackTrace();
			if (mListener != null) {
				mListener.onVoiceCode(ERequestState.Failed);
			}
		} finally {
			if (headers != null) {
				headers.clear();
				headers = null;
			}
		}
	}

	
	private void parseVerifyCodeBody(XmlPullParser xmlParser, Response response)  throws Exception {
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			if (tagName.equals("accountSid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			} else if (tagName.equals("callSid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			}  else if (tagName.equals("dateCreated")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			}  else if (tagName.equals("status")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			}  else if (tagName.equals("to")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			}  else if (tagName.equals("uri")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			}   else if (tagName.equals("verifyCode")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					
				}
			}else {
				xmlParser.nextText();
			}
		}
		
	}

	private static void parseLandingCallBody(XmlPullParser xmlParser, LandingCall response) throws Exception {
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			if (tagName.equals("callSid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.callSid = text.trim();
				}
			} else if (tagName.equals("dateCreated")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.dateCreated = text.trim();
				}
			} else {
				xmlParser.nextText();
			}
		}
	}

	public void setOnRestHelperListener(onRestHelperListener mListener) {
		this.mListener = mListener;
	}

	public interface onRestHelperListener {
		void onLandingCAllsStatus(ERequestState reason , LandingCall landingCall);

		void onVoiceCode(ERequestState reason);
	}

	@Override
	protected void formatURL(StringBuffer requestUrl, int requestType) {
		switch (requestType) {
		case KEY_VERIFY_CODE:
			requestUrl.append("Calls/VoiceVerify");
			break;
			
		case KEY_LANDING_CALL:
			requestUrl.append("Calls/LandingCalls");
			
			break;
		default:
			break;
		}
	}

	@Override
	protected void handleParserXMLBody(int parseType, XmlPullParser xmlParser,
			Response response) throws Exception {
		if (parseType == KEY_LANDING_CALL) {
			parseLandingCallBody(xmlParser, (LandingCall)response);
		} else if (parseType == KEY_VERIFY_CODE) {
			parseVerifyCodeBody(xmlParser, response);
		} else {
			xmlParser.nextText();
		}
	}

	
	/* (non-Javadoc)
	 * @see com.voice.demo.tools.net.BaseRestHelper#getResponseByKey(int)
	 */
	@Override
	protected Response getResponseByKey(int requestType) {
		if(requestType == KEY_LANDING_CALL ) {
			return new LandingCall();
		}
		return new Response();
	}
}
