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
 */package com.voice.demo.tools.net;

import java.io.InputStream;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserFactory;

import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.webkit.URLUtil;
import android.widget.Toast;

import com.hisun.phone.core.voice.Build;
import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.token.Base64;
import com.hisun.phone.core.voice.util.VoiceUtil;
import com.voice.demo.CCPApplication;
import com.voice.demo.group.RestGroupManagerHelper;
import com.voice.demo.tools.CCPConfig;

public abstract class BaseRestHelper {

	public static final String TAG = RestGroupManagerHelper.class.getSimpleName();
	
	/**
	 * Rest version code
	 */
	public static final String REST_VERSION = "/" + Build.REST_VERSION;
	
	public RequestHandler mRequestHandler = new RequestHandler();;
	
	/**
	 * 
	 * @param requestType
	 * @param formatTimestamp
	 * @return
	 */
	public HashMap<String, String> getSubAccountRequestHead(int requestType, String formatTimestamp){

		return requestHead(AccountType.Sub_Account, formatTimestamp);
	}
	
	/**
	 * 
	 * @param requestType
	 * @param formatTimestamp
	 * @return
	 */
	public HashMap<String, String> getAccountRequestHead(int requestType, String formatTimestamp){
		
		return requestHead(AccountType.Main_Account, formatTimestamp);
	}
	
	/**
	 * 
	 * @param type
	 * @param formatTimestamp
	 * @return
	 */
	HashMap<String, String> requestHead(AccountType type , String formatTimestamp) {
		
		HashMap<String, String> headers = new HashMap<String, String>();
		headers.put("Accept", "application/xml");
		headers.put("Content-Type", "application/xml;charset=utf-8");
		
		if(TextUtils.isEmpty(formatTimestamp)) {
			return headers;
		}
		
		String authorization = null;
		
		if(type == AccountType.Main_Account) {
			authorization = Base64
			.encode((CCPConfig.Main_Account + ":" + formatTimestamp)
					.getBytes());
		} else {
			authorization = Base64
			.encode((CCPConfig.Sub_Account + ":" + formatTimestamp)
					.getBytes());
		}
		
		headers.put("Authorization", authorization);
		return headers;
	}
	
	
	/**
	 * 
	 * @param type
	 * @param formatTimestamp
	 * @return
	 */
	public String getRequestUrlSig(AccountType type , String formatTimestamp) {
		if(type == AccountType.Main_Account) {
			return VoiceUtil.md5(CCPConfig.Main_Account
					+ CCPConfig.Main_Token + formatTimestamp);
		} else  {
			
			return VoiceUtil.md5(CCPConfig.Sub_Account
					+ CCPConfig.Sub_Token + formatTimestamp);
		}
	}
	
	/**
	 * 
	 * @return
	 */
	public String getTimestamp() {
		
		return VoiceUtil.formatTimestamp(System.currentTimeMillis());
	}
	
	
	public Response doParserResponse(int parseType,InputStream is) throws Exception {
		if (is == null) {
			throw new IllegalArgumentException("resource is null.");
		}
		XmlPullParser xmlParser = XmlPullParserFactory.newInstance().newPullParser();
		Response response = null;
		try {
			xmlParser.setInput(is, null);
			xmlParser.nextTag();
			String rootName = xmlParser.getName();
			if (!isRootNode(rootName)) {
				throw new IllegalArgumentException("xml root node is invalid.");
			}

			response = getResponseByKey(parseType);
			xmlParser.require(XmlPullParser.START_TAG, null, rootName);
			while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
				String tagName = xmlParser.getName();
				if (tagName != null && tagName.equals("statusCode")) {
					String text = xmlParser.nextText();
					response.statusCode = text;
				} else if (tagName != null && tagName.equals("statusMsg")) {
					String text = xmlParser.nextText();
					response.statusMsg = text;
				} else { 
					handleParserXMLBody(parseType ,xmlParser, response);
					
				}
			}
			xmlParser.require(XmlPullParser.END_TAG, null, rootName);
			xmlParser.next();
			xmlParser.require(XmlPullParser.END_DOCUMENT, null, null);
			print(response);
		} catch (Exception e) {
			e.printStackTrace();
			if (response != null) {
				response.released();
				response = null;
			}
			throw new Exception("parse xml occur errors:" + e.getMessage());
		} finally {
			if (is != null) {
				is.close();
				is = null;
			}
			xmlParser = null;
		}

		if(response != null && response.isError()) {
			handleRequestFailed(parseType, response.statusCode, response.getStatusMsg());
		}
		return response;
	}
	
	/**
	 * 
	 * @param requestType
	 * @param formatTimestamp
	 * @return
	 */
	public StringBuffer getSubAccountRequestURL(int requestType, String formatTimestamp){
		StringBuffer requestUrl = requestURL(requestType, formatTimestamp , AccountType.Sub_Account);
		
		return requestUrl;
	}

	/**
	 * MD5 (the main account ID + main account authorization token + timestamp
	 * @param requestType
	 * @param formatTimestamp
	 * @param accountType
	 * @return
	 */
	StringBuffer requestURL(int requestType, String formatTimestamp,
			AccountType accountType) {
		
		StringBuffer requestUrl = getServerUrl(CCPConfig.REST_SERVER_ADDRESS);
		if (accountType == AccountType.Sub_Account) {
			requestUrl.append("/SubAccounts/").append(CCPConfig.Sub_Account).append("/");
		} else {
			requestUrl.append("/Accounts/").append(CCPConfig.Main_Account).append("/");
		}

		formatURL(requestUrl, requestType);
		requestUrl.append("?sig=").append(getRequestUrlSig(accountType, formatTimestamp));
		return requestUrl;
	}

	/**
	 * 
	 * @return
	 */
//	public StringBuffer getServerUrl(boolean sandbox) {
//		
//		if(sandbox) {
//			return getSandboxServerUrl();
//		}
//		return getServerUrl("app.cloopen.com");
//	}
	
	public StringBuffer getSandboxServerUrl() {
		return getServerUrl(CCPConfig.REST_SERVER_ADDRESS);
	}
	
	private StringBuffer getServerUrl(String server) {
		// 
		if(!CCPApplication.getInstance().isDeveloperMode()&&"app.cloopen.com".equals(server)) {
			CCPConfig.REST_SERVER_PORT = "8883";
			server = "sandboxapp.cloopen.com";
		}
		
		StringBuffer requestUrl = new StringBuffer("https://"
				+ server + ":"
				+ CCPConfig.REST_SERVER_PORT + REST_VERSION);
		if(checkHttpScheme(server)) {
			requestUrl = new StringBuffer(server + ":"
					+ CCPConfig.REST_SERVER_PORT + REST_VERSION);
		}
		return requestUrl;
	}
	
	/**
	 * @return
	 */
	private boolean checkHttpScheme(String server) {
		if(!TextUtils.isEmpty(server) 
				&& !server.toLowerCase().startsWith("https://")
				&& !server.toLowerCase().startsWith("http://")) {
			return false;
		}
		return true;
	}

	
	/**
	 * @param requestType
	 * @param formatTimestamp
	 * @return
	 */
	public StringBuffer getAccountRequestURL(int requestType, String formatTimestamp) {
		
		StringBuffer requestUrl = requestURL(requestType, formatTimestamp , AccountType.Main_Account);
		
		return requestUrl;
	}
	
	/**
	 * 
	 * @return
	 */
	protected StringBuffer getDefaultRequestURL() {
		
		StringBuffer requestUrl = getServerUrl("app.cloopen.com");
		requestUrl.append("/General/GetDemoAccounts");
		return requestUrl;
	}
	
	protected void checkHttpUrl(String url) {
		// check url
		if (!URLUtil.isHttpsUrl(url) && !URLUtil.isHttpUrl(url)) {
			throw new RuntimeException("address invalid.");
		}
	}
	
	/**
	 * 
	 * @param key
	 * @return
	 */
	protected abstract Response getResponseByKey(int requestType);
	
	/**
	 * 
	 * @param requestUrl
	 * @param requestType
	 */
	protected abstract void formatURL(StringBuffer requestUrl , int requestType);
	
	/**
	 * 
	 * @param xmlParser
	 * @param response
	 */
	protected abstract void handleParserXMLBody(int parseType , XmlPullParser xmlParser, Response response)throws Exception ;
	
	/**
	 * 
	 * @param requestType
	 * @param errorCode
	 * @param errorMessage
	 */
	protected void handleRequestFailed(int requestType ,final String errorCode , final String errorMessage) {
		
		if(mRequestHandler != null) {
			mRequestHandler.post(new Runnable() {
				
				@Override
				public void run() {
					Toast.makeText(CCPApplication.getInstance().getApplicationContext(), errorMessage + "[" +errorCode+ "]", Toast.LENGTH_LONG).show();
				}
			});
		}
	}
	
	
	/**
	 * 
	 * @param rootName
	 * @return
	 */
	boolean isRootNode(String rootName) {
		if (rootName == null || (!rootName.equalsIgnoreCase("Response"))) {
			return false;
		}
		return true;
	}
	
	/**
	 * 
	 * @param r
	 */
	void print(Response r) {
		if (r != null) {
			r.print();
		}
	}
	
	enum AccountType {
		Main_Account , Sub_Account
	}
	
	protected interface BaseHelpListener {
		void onRequestFailed(int requestType ,int errorCode , String errorMessage);
	}
	
	public static class RequestHandler extends Handler{
		@Override
		public void handleMessage(Message msg) {
			
			super.handleMessage(msg);
		}
	}
}
