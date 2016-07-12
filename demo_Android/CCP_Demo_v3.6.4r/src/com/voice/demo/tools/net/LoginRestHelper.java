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
package com.voice.demo.tools.net;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.text.TextUtils;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.model.setup.SubAccount;
import com.hisun.phone.core.voice.net.HttpManager;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;
import com.voice.demo.CCPApplication;
import com.voice.demo.tools.CCPDES3Utils;
import com.voice.demo.ui.model.Application;
import com.voice.demo.ui.model.DemoAccounts;

/**
 * @ClassName: BaseRestHelper.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-17
 * @version 3.6
 */
public class LoginRestHelper extends BaseRestHelper {
	
	public static final String TAG = LoginRestHelper.class.getSimpleName();
	
	/**
	 * 
	 */
	public static final int REST_CLIENT_LOGIN = 0X1;
	
	/**
	 * 
	 */
	public static final int REST_BUILD_TEST_NUMBER= TaskKey.TASK_KEY_BUILD_NUMNBER;
	
	private static LoginRestHelper mInstance = null;
	
	private OnRestManagerHelpListener mListener;
	
	public static LoginRestHelper getInstance(){
		if (mInstance == null) {
			mInstance = new LoginRestHelper();
		}
		return mInstance;
	}
	
	private LoginRestHelper(){
		super();
	}
	
	/**
	 * 
	 * @param userName
	 * @param userPass
	 */
	public void doClientLoginRequest(String userName , String userPass) {
		int keyValue = REST_CLIENT_LOGIN;
		
		StringBuffer requestURL = getDefaultRequestURL();
		Log4Util.w(Device.TAG, "url: " + requestURL + "\r\n");
		
		HashMap<String, String> requestHead = getSubAccountRequestHead(keyValue, null);
	
		final StringBuffer requestBody = new StringBuffer("<Request>\r\n");
		requestBody.append("\t<user_name>").append(userName).append("</user_name>\r\n");
		requestBody.append("\t<user_pwd>").append(userPass).append("</user_pwd>\r\n");
		requestBody.append("</Request>\r\n");
	
		try {
			
			String decodeQES = CCPDES3Utils.encode(requestBody.toString());
			String xml = HttpManager.httpPost(requestURL.toString(), requestHead,
					decodeQES);
			
			Log4Util.w(Device.TAG, xml + "\r\n");

			Response response = doParserResponse(keyValue, new ByteArrayInputStream(xml.getBytes()));
			
			DemoAccounts demoAccounts = null;
			if (response != null) {
				if (response.isError()) {
					handleRequestFailed(keyValue, response.statusCode , response.statusMsg);
				} else {
					demoAccounts = (DemoAccounts) response;
					CCPApplication.getInstance().putDemoAccounts(demoAccounts);
					if (mListener != null){
						mListener.onClientLoginRequest(demoAccounts);
					}
				}
			} else {
				handleRequestFailed(keyValue, SdkErrorCode.SDK_XML_ERROR + "" , null);
			}

			

			
		} catch (Exception e) {
			e.printStackTrace();
			handleRequestFailed(keyValue, SdkErrorCode.SDK_UNKNOW_ERROR + "" , null);
		} finally {
			if (requestHead != null) {
				requestHead.clear();
				requestHead = null;
			}
		}
	}
	
	/**
	 * 
	 * @param oldNum
	 * @param newNum
	 */
	public void doTestNumber(String oldNum , String newNum) {
		int keyValue = REST_BUILD_TEST_NUMBER;
		
		StringBuffer requestURL = getAccountRequestURL(keyValue, getTimestamp());
		Log4Util.w(Device.TAG, "url: " + requestURL + "\r\n");
		
		HashMap<String, String> requestHead = getAccountRequestHead(keyValue, getTimestamp());
	
		final StringBuffer requestBody = new StringBuffer("<Request>\r\n");
		if(!TextUtils.isEmpty(oldNum))
			requestBody.append("\t<oldNum>").append(oldNum).append("</oldNum>\r\n");
		requestBody.append("\t<newNum>").append(newNum).append("</newNum>\r\n");
		requestBody.append("</Request>\r\n");
	
		try {
			/*String encode = CCPDES3Utils.encode(requestBody.toString());
			Log4Util.i(TAG, encode);
			
			Log4Util.i(TAG, CCPDES3Utils.decode(encode));*/
			String xml = HttpManager.httpPost(requestURL.toString(), requestHead,
					requestBody.toString());
			
			Log4Util.w(Device.TAG, xml + "\r\n");

			Response response = doParserResponse(keyValue, new ByteArrayInputStream(xml.getBytes()));
			
			if (response != null) {
				if (response.isError()) {
					handleRequestFailed(keyValue, response.statusCode , response.statusMsg);
				} else {
					if (mListener != null) {
						mListener.onTestNumber(newNum);
					}
				}
			} else {
				handleRequestFailed(keyValue, SdkErrorCode.SDK_XML_ERROR + "" , null);
			}
			
		} catch (Exception e) {
			e.printStackTrace();
			handleRequestFailed(keyValue, SdkErrorCode.SDK_UNKNOW_ERROR + "" , null);
		} finally {
			if (requestHead != null) {
				requestHead.clear();
				requestHead = null;
			}
		}
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.tools.net.RestRequestManagerHelper#getResponseByKey(int)
	 */
	@Override
	protected Response getResponseByKey(int key) {
		switch (key) {
		case REST_CLIENT_LOGIN:
			
			return new DemoAccounts();

		default:
			return new Response();
		}
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.tools.net.RestRequestManagerHelper#formatURL(java.lang.StringBuffer, int)
	 */
	@Override
	protected void formatURL(StringBuffer requestUrl, int requestType) {
		switch (requestType) {
		case REST_CLIENT_LOGIN:
			requestUrl.append("GetServiceNum");
			break;
			
		case REST_BUILD_TEST_NUMBER :
			requestUrl.append("TestNumEdit");
			break;
		default:
			break;
		}
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.tools.net.RestRequestManagerHelper#handleParserXMLBody(int, org.xmlpull.v1.XmlPullParser, com.hisun.phone.core.voice.model.Response)
	 */
	@Override
	protected void handleParserXMLBody(int parseType, XmlPullParser xmlParser,
			Response response) throws Exception {
		if(parseType == REST_CLIENT_LOGIN) {
			
			doParserClientLogin(xmlParser, (DemoAccounts)response);
		} else {
			xmlParser.nextText();
		}
	}
	
	public void doParserClientLogin(XmlPullParser xmlParser,
			DemoAccounts response) throws Exception{
		
		String ptagName = xmlParser.getName();
		if (ptagName.equals("Application")) {
			Application application = new Application();
			while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
				doParserApplications(xmlParser, application);
			}
			response.putApplication(application);
		} else if (ptagName.equals("main_account")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setMainAccount(text);
			}
		} else if (ptagName.equals("main_token")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setMainToken(text);
			}
		} else if (ptagName.equals("mobile")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setMobile(text);
			}
		} else if (ptagName.equals("test_number")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setTestNumber(text);
			}
		} else if (ptagName.equals("nickname")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setNickname(text);
			}
		} else {
			xmlParser.nextText();
		}
	}

	/**
	 * 
	 * @param xmlParser
	 * @param application
	 * @throws XmlPullParserException
	 * @throws IOException
	 */
	private void doParserApplications(XmlPullParser xmlParser,
			Application application) throws XmlPullParserException, IOException {
 		String ptagName = xmlParser.getName();
		if (ptagName.equals("SubAccount")) {
			SubAccount subAccount = new SubAccount();
			while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
				doParserSubAccount(xmlParser, subAccount);
			}
			application.putSubAccount(subAccount);
		} else if (ptagName.equals("appId")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				application.setAppId(text);
			}
		} else if (ptagName.equals("friendlyName")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				application.setFriendlyName(text);
			}
		} else {
			xmlParser.nextText();
		}
	}

	/**
	 * 
	 * @param xmlParser
	 * @param subAccount
	 * @throws XmlPullParserException
	 * @throws IOException
	 */
	private void doParserSubAccount(XmlPullParser xmlParser,
			SubAccount subAccount) throws XmlPullParserException, IOException {
		String tagName = xmlParser.getName();
		if (tagName.equals("sub_account")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				subAccount.accountSid = text;
			}
		} else if(tagName.equals("sub_token")){
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				subAccount.authToken = text;
			}
		}  else if(tagName.equals("voip_account")){
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				subAccount.sipCode = text;
			}
		} else if(tagName.equals("voip_token")){
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				subAccount.sipPwd = text;
			}
		} else {
			xmlParser.nextText();
		}
	}

	
	public void setOnRestManagerHelpListener(OnRestManagerHelpListener l) {
		this.mListener = l;
	}
	
	/**
	 * 
	 * 
	 * @ClassName: BaseRestHelper.java
	 * @Description: TODO
	 * @author Jorstin Chan 
	 * @date 2013-12-16
	 * @version 3.6
	 */
	public interface OnRestManagerHelpListener extends BaseHelpListener{
		void onClientLoginRequest(DemoAccounts demoAccounts);
		void onTestNumber(String number);
	}

	/* (non-Javadoc)
	 * @see RestRequestManagerHelper#RequestFailed(int, Response)
	 */
	@Override
	protected void handleRequestFailed(int requestType ,String errorCode , String errorMessage) {
		
		if(mListener != null) {
			
			int code = -1;
			try {
				code = Integer.valueOf(errorCode);
			} catch (Exception e) {
				
			}
			if(TextUtils.isEmpty(errorMessage)) {
				errorMessage = "请求失败,";
			}
			mListener.onRequestFailed(requestType, code, errorMessage);
		}
	}
	
	
}
