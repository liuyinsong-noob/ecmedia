package com.hisun.phone.core.voice.net;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.zip.GZIPInputStream;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.HttpVersion;
import org.apache.http.StatusLine;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.conn.ClientConnectionManager;
import org.apache.http.conn.ConnectTimeoutException;
import org.apache.http.conn.params.ConnRoutePNames;
import org.apache.http.conn.scheme.PlainSocketFactory;
import org.apache.http.conn.scheme.Scheme;
import org.apache.http.conn.scheme.SchemeRegistry;
import org.apache.http.conn.ssl.SSLSocketFactory;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.impl.conn.tsccm.ThreadSafeClientConnManager;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.params.HttpProtocolParams;
import org.apache.http.protocol.HTTP;

import android.text.TextUtils;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.exception.CCPException;
import com.hisun.phone.core.voice.exception.CCPHttpException;
import com.hisun.phone.core.voice.model.CCPParameters;
import com.hisun.phone.core.voice.util.CheckApnTypeUtils;
import com.hisun.phone.core.voice.util.Cryptos;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;
import com.hisun.phone.core.voice.util.VoiceUtil;

public final class HttpManager {
	
	private static final String TAG = "HttpManager";
	
	public static final String HTTPMETHOD_GET = "GET";
	public static final String HTTPMETHOD_POST = "POST";

	private static final String BOUNDARY = getBoundry();
	private static final String MP_BOUNDARY = "--" + BOUNDARY;
	private static final String END_MP_BOUNDARY = "--" + BOUNDARY + "--";
	private static final String MULTIPART_FORM_DATA = "multipart/form-data";

	private static int sslPort = 0;
	public static final int SKIP_CACHE_NUM = 128;

	private static final int SET_CONNECTION_TIMEOUT = 45 * 1000;
	private static final int SET_SOCKET_TIMEOUT = 30 * 1000;
    
    static HttpClient getNewHttpClient() {
    	
    	return newHttpClient();
    }

	static HttpClient newHttpClient() {
		try {
			KeyStore trustStore = KeyStore.getInstance(KeyStore.getDefaultType());
			trustStore.load(null, null);

			SSLSocketFactory sf = new SSLSocketFactoryEx(trustStore);
			sf.setHostnameVerifier(SSLSocketFactory.ALLOW_ALL_HOSTNAME_VERIFIER);

			HttpParams params = new BasicHttpParams();
			
			HttpConnectionParams.setConnectionTimeout(params, 10000);
            HttpConnectionParams.setSoTimeout(params, 10000);
			
			HttpProtocolParams.setVersion(params, HttpVersion.HTTP_1_1);
			HttpProtocolParams.setContentCharset(params, HTTP.UTF_8);

			SchemeRegistry registry = new SchemeRegistry();
			registry.register(new Scheme("http", PlainSocketFactory.getSocketFactory(), 80));
			registry.register(new Scheme("https", sf, (sslPort <= 0 ? 8883 : sslPort)));

			ClientConnectionManager ccm = new ThreadSafeClientConnManager(params, registry);
			
			HttpConnectionParams.setConnectionTimeout(params, SET_CONNECTION_TIMEOUT);
			HttpConnectionParams.setSoTimeout(params, SET_SOCKET_TIMEOUT);
			return new DefaultHttpClient(ccm, params);
		} catch (Exception e) {
			Log4Util.e(Device.TAG, e.toString());
			return new DefaultHttpClient();
		}
	}

	static class SSLSocketFactoryEx extends SSLSocketFactory {

		SSLContext sslContext = SSLContext.getInstance("TLS");

		public SSLSocketFactoryEx(KeyStore truststore) throws NoSuchAlgorithmException, KeyManagementException,
				KeyStoreException, UnrecoverableKeyException {
			super(truststore);

			TrustManager tm = new X509TrustManager() {

				public java.security.cert.X509Certificate[] getAcceptedIssuers() {
					return null;
				}

				@Override
				public void checkClientTrusted(
						java.security.cert.X509Certificate[] chain,
						String authType)
						throws java.security.cert.CertificateException {

				}

				@Override
				public void checkServerTrusted(
						java.security.cert.X509Certificate[] chain,
						String authType)
						throws java.security.cert.CertificateException {

				}
			};
			sslContext.init(null, new TrustManager[] { tm }, null);
		}

		@Override
		public Socket createSocket(Socket socket, String host, int port,
				boolean autoClose) throws IOException, UnknownHostException {
			return sslContext.getSocketFactory().createSocket(socket, host,
					port, autoClose);
		}

		@Override
		public Socket createSocket() throws IOException {
			return sslContext.getSocketFactory().createSocket();
		}
	}

	private static String stringFromInputStream(InputStream is)
			throws IOException {
		char[] buf = new char[1024];
		StringBuilder out = new StringBuilder();
		Reader in = new InputStreamReader(is, "UTF-8");
		int bin = 0;
		while ((bin = in.read(buf, 0, buf.length)) >= 0) {
			out.append(buf, 0, bin);
		}
		in.close();
		return out.toString();
	}

	public static String httpGet(String url, HashMap<String, String> headers)
			throws Exception {
		HttpClient httpClient = getNewHttpClient();
		
		HttpGet request = new HttpGet(url);
		Iterator<String> keys = headers.keySet().iterator();
		while (keys.hasNext()) {
			String key = keys.next();
			String value = headers.get(key);
			request.setHeader(key, value);
		}

		HttpResponse response = httpClient.execute(request);
		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			if (statusCode == HttpStatus.SC_OK) {
				Log4Util.d(TAG, "httpGet success.");
				return stringFromInputStream(response.getEntity().getContent());
			} else {
				throw new Exception("Got error code " + statusCode + " from server.");
			}
		}

		throw new Exception("Unable to connect to server.");
	}

	public static String httpPost(final String url, final HashMap<String, String> headers,
			final String requestBody) throws Exception {
		HttpClient httpClient = getNewHttpClient();

		HttpPost httpRequest = new HttpPost(url);
		Iterator<String> keys = headers.keySet().iterator();
		while (keys.hasNext()) {
			String key = keys.next();
			String value = headers.get(key);
			httpRequest.setHeader(key, value);
		}
		
		if(requestBody != null) {
			HttpEntity entity = new ByteArrayEntity(requestBody.getBytes());
			Log4Util.d(Device.TAG, requestBody);
			
			httpRequest.setEntity(entity);
		}
		HttpResponse response = httpClient.execute(httpRequest);

		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			Log4Util.d(Device.TAG, "httpPost request return " + statusCode);
			if (statusCode == HttpStatus.SC_OK) {
				return stringFromInputStream(response.getEntity().getContent());
			} else {
				throw new Exception("Got error code " + statusCode + " from server.");
			}
		}

		throw new Exception("Unable to connect to server.");
	}
	
	public static String httpPost(final String url) throws Exception {
		HttpClient httpClient = getNewHttpClient();
		
		HttpPost httpRequest = new HttpPost(url);
		HttpResponse response = httpClient.execute(httpRequest);
		
		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			Log4Util.d(Device.TAG, "httpPost request return " + statusCode);
			if (statusCode == HttpStatus.SC_OK) {
				return stringFromInputStream(response.getEntity().getContent());
			} else {
				throw new Exception("Got error code " + statusCode + " from server.");
			}
		}
		
		throw new Exception("Unable to connect to server.");
	}
	
	
	public static String httpUploadFile(String url, HashMap<String, String> headers,
			byte[] requestBody) throws Exception {
		HttpClient httpClient = getNewHttpClient();

		HttpPost httpRequest = new HttpPost(url);
		Iterator<String> keys = headers.keySet().iterator();
		while (keys.hasNext()) {
			String key = keys.next();
			String value = headers.get(key);
			httpRequest.setHeader(key, value);
		}
		
		HttpEntity entity = new ByteArrayEntity(requestBody);
		Log4Util.d(Device.TAG, requestBody.toString());
		httpRequest.setEntity(entity);
		HttpResponse response = httpClient.execute(httpRequest);

		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			Log4Util.w(Device.TAG, "httpPost request return " + statusCode);
			if (statusCode == HttpStatus.SC_OK) {
				return stringFromInputStream(response.getEntity().getContent());
			} else {
				throw new Exception("Got error code " + statusCode + " from server.");
			}
		}

		throw new Exception("Unable to connect to server.");
	}

	/**
	 * @return the sslPort
	 */
	public static int getSSLPort() {
		return sslPort;
	}

	/**
	 * @param sslPort the sslPort to set
	 */
	public static void setSSLPort(int port) {
		sslPort = port;
		Log4Util.d(Device.TAG, "HttpHelper ssl port: " + sslPort);
	}
	
	
	public static boolean httpDowload(String url , String svaePath) throws Exception {
		HttpClient httpClient = getNewHttpClient();

		HttpGet request = new HttpGet(url);

		String fileName = url.substring(url.lastIndexOf("/") + 1, url.length());
		
		if(fileName == null){
			throw new Exception("This download url is invalid.");
		}
		
		HttpResponse response = httpClient.execute(request);
		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			if (statusCode == HttpStatus.SC_OK) {
				Log4Util.w(TAG, "dowload file from " + url + " success.");
				return saveFromInputStream(response.getEntity().getContent() , svaePath);
			} else {
				throw new Exception("Got error code " + statusCode + " from server.");
			}
		}

		throw new Exception("Unable to connect to server.");
	}
	
	public static int httpDowloadFile(String url , String svaePath) throws Exception {
		HttpClient httpClient = getNewHttpClient();
		
		HttpGet request = new HttpGet(url);
		
		String fileName = url.substring(url.lastIndexOf("/") + 1, url.length());
		
		if(fileName == null){
			throw new Exception("This download url is invalid.");
		}
		
		HttpResponse response = httpClient.execute(request);
		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			if (statusCode == HttpStatus.SC_OK) {
				Log4Util.w(TAG, "dowload file from " + url + " success.");
				boolean result = saveFromInputStream(response.getEntity().getContent() , svaePath);;
				return result == true ? 0 : SdkErrorCode.SDK_WRITE_FAILED;
			} else if (statusCode == HttpStatus.SC_NOT_FOUND) {
				return SdkErrorCode.SDK_FILE_NOTEXIST;
			} else {
				Log4Util.d(Device.TAG, "Got error code " + statusCode + " from server.");
				return statusCode;
				//throw new Exception("Got error code " + statusCode + " from server.");
			}
		}
		
		throw new Exception("Unable to connect to server.");
	}

	private static boolean saveFromInputStream(InputStream content , String fileName) {
		try{
			FileOutputStream out = new FileOutputStream(fileName);

			byte[] b = new byte[1024 * SKIP_CACHE_NUM];
			int len = 0;
			while ((len = content.read(b)) != -1) {
				out.write(b, 0, len);
			}
			out.flush();
			out.close();
		} catch (IOException e) {
			e.printStackTrace();
			return false;
		}
		return true;
	}
	
	
	public static String httpPut(String url, HashMap<String, String> headers,
			String requestBody) throws Exception {
		HttpClient httpClient = getNewHttpClient();

		HttpPut httpRequest = new HttpPut(url);
		Iterator<String> keys = headers.keySet().iterator();
		while (keys.hasNext()) {
			String key = keys.next();
			String value = headers.get(key);
			httpRequest.setHeader(key, value);
		}
		HttpEntity entity = new ByteArrayEntity(requestBody.getBytes());
		Log4Util.d(Device.TAG, requestBody);
		
		httpRequest.setEntity(entity);
		HttpResponse response = httpClient.execute(httpRequest);

		if (response != null) {
			int statusCode = response.getStatusLine().getStatusCode();
			Log4Util.w(Device.TAG, "HttpPut request return " + statusCode);
			if (statusCode == HttpStatus.SC_OK) {
				return stringFromInputStream(response.getEntity().getContent());
			} else {
				throw new Exception("Got error code " + statusCode + " from server.");
			}
		}

		throw new Exception("Unable to connect to server.");
	}
	
	/**
	 * @param url
	 * @param params
	 * @return
	 * @throws CCPException 
	 */
	public static String doRequestPostUrl(String url, CCPParameters params) throws CCPException {
		
		return doRequestUrl(url, HTTPMETHOD_POST, params);
	}
	
	/**
	 * @param url
	 * @param method
	 * @param params
	 * @return
	 * @throws CCPException
	 */
	public static String doRequestUrl(String url, String method,
			CCPParameters params) throws CCPException {
		
		return doRequestUrl(url, method, params, null , false);
	}
	
	/**
	 * @param url
	 * @param method
	 * @param params
	 * @return
	 * @throws CCPException
	 */
	public static String doRequestUrl(String url, String method,
			CCPParameters params , boolean encrypt) throws CCPException {
		
		return doRequestUrl(url, method, params, null , encrypt);
	}
	
	/**
	 * 
	 * @param url
	 * @param method
	 * @param params
	 * @param file
	 * @return
	 * @throws CCPException
	 */
	public static String doRequestUrl(String url, String method,
			CCPParameters params, String file , boolean encrypt) throws CCPException {

		String result = "";
		try {
			HttpClient client = getNewHttpClient();
			HttpUriRequest request = null;
			ByteArrayOutputStream bos = null;
			client.getParams().setParameter(ConnRoutePNames.DEFAULT_PROXY,
					CheckApnTypeUtils.getHttpHost());
			if (method.equals(HTTPMETHOD_GET)) {
				if(params.size() > 1) {
					url = url + "?" + VoiceUtil.encodeUrl(params);
				}
				Log4Util.w(Device.TAG, "doRequestUrl SDK request Url : \t\n" + url);
				HttpGet get = new HttpGet(url);
				request = get;
			} else if (method.equals(HTTPMETHOD_POST)) {
				String _sig = params.getValue("sig");
				if(!TextUtils.isEmpty(_sig)) {
					params.remove("sig");
					url = url + "?sig=" + _sig;
				}
				Log4Util.w(Device.TAG, "doRequestUrl SDK request Url : \t\n" + url);
				HttpPost post = new HttpPost(url);
				request = post;
				byte[] data = null;
				String _contentType = params.getValue("content-type");

				bos = new ByteArrayOutputStream();
				if (!TextUtils.isEmpty(file)) {
					paramToUpload(bos, params);
					post.setHeader("Content-Type", MULTIPART_FORM_DATA
							+ "; boundary=" + BOUNDARY);
					imageContentToUpload(bos, file);
				} else {
					if (_contentType != null) {
						params.remove("content-type");
						post.setHeader("Content-Type", _contentType);
					} else {
						post.setHeader("Content-Type",
								"application/x-www-form-urlencoded");
					}

					String postParam = VoiceUtil.buildRequestBody(params);
					if(encrypt) {
						postParam = Cryptos.toBase64QES(Cryptos.SECRET_KEY, postParam);
					}
					data = postParam.getBytes("UTF-8");
					bos.write(data);
				}
				data = bos.toByteArray();
				bos.close();
				ByteArrayEntity formEntity = new ByteArrayEntity(data);
				post.setEntity(formEntity);
			} else if (method.equals("DELETE")) {
				request = new HttpDelete(url);
			}
			
			buildHttpRequestHeader(params, request);
			
			HttpResponse response = client.execute(request);
			StatusLine status = response.getStatusLine();
			int statusCode = status.getStatusCode();

			if (statusCode != 200) {
				result = readHttpResponse(response);
				throw new CCPHttpException(result, statusCode);
			}
			result = readHttpResponse(response);
			return result;
		} catch (ConnectTimeoutException e) {
			throw new CCPHttpException(e.getMessage(), SdkErrorCode.SDK_REQUEST_TIMEOUT);
		} catch (IOException e) {
			throw new CCPHttpException(e.getMessage(), SdkErrorCode.SDK_HTTP_ERROR);
		} catch (Exception e) {
			throw new CCPException();
		}
	}

	/**
	 * @param params
	 * @param request
	 */
	private static void buildHttpRequestHeader(CCPParameters params,
			HttpUriRequest request) {
		String _authorization = params.getValue("Authorization");
		if(!TextUtils.isEmpty(_authorization)) {
			request.setHeader("Accept", "application/xml");
			request.setHeader("Content-Type", "application/xml;charset=utf-8");
			request.setHeader("Authorization", _authorization);
		}
	}
	
	/**
	 * @param url
	 * @param method
	 * @param params
	 * @param file
	 * @return
	 * @throws CCPException
	 */
	public static String doUploadFile(String url, String method,
			CCPParameters params, String file) throws CCPException {
		String result = "";
		try {
			HttpClient client = getNewHttpClient();
			HttpUriRequest request = null;
			ByteArrayOutputStream bos = null;
			client.getParams().setParameter(ConnRoutePNames.DEFAULT_PROXY,
					CheckApnTypeUtils.getHttpHost());
			if (method.equals(HTTPMETHOD_GET)) {
				url = url + "?" + VoiceUtil.encodeUrl(params);
				HttpGet get = new HttpGet(url);
				request = get;
			} else if (method.equals(HTTPMETHOD_POST)) {
				HttpPost post = new HttpPost(url);
				request = post;
				byte[] data = null;
				String _contentType = params.getValue("content-type");

				bos = new ByteArrayOutputStream();
				if (!TextUtils.isEmpty(file)) {
					paramToUpload(bos, params);
					post.setHeader("Content-Type", MULTIPART_FORM_DATA
							+ "; boundary=" + BOUNDARY);
					fileToUpload(bos, file);
				} else {
					if (_contentType != null) {
						params.remove("content-type");
						post.setHeader("Content-Type", _contentType);
					} else {
						post.setHeader("Content-Type",
								"application/x-www-form-urlencoded");
					}

					String postParam = VoiceUtil.encodeParameters(params);
					data = postParam.getBytes("UTF-8");
					bos.write(data);
				}
				data = bos.toByteArray();
				bos.close();
				ByteArrayEntity formEntity = new ByteArrayEntity(data);
				post.setEntity(formEntity);
			} else if (method.equals("DELETE")) {
				request = new HttpDelete(url);
			}
			HttpResponse response = client.execute(request);
			StatusLine status = response.getStatusLine();
			int statusCode = status.getStatusCode();

			if (statusCode != 200) {
				result = readHttpResponse(response);
				throw new CCPHttpException(result, statusCode);
			}
			result = readHttpResponse(response);
			return result;
		} catch (IOException e) {
			throw new CCPException(e);
		}
	}

	
	/**
	 * 
	 * @param baos
	 * @param params
	 * @throws CCPException
	 */
	private static void paramToUpload(OutputStream baos, CCPParameters params)
			throws CCPException {
		String key = "";
		for (int loc = 0; loc < params.size(); loc++) {
			key = params.getKey(loc);
			StringBuilder temp = new StringBuilder(10);
			temp.setLength(0);
			temp.append(MP_BOUNDARY).append("\r\n");
			temp.append("content-disposition: form-data; name=\"").append(key)
					.append("\"\r\n\r\n");
			temp.append(params.getValue(key)).append("\r\n");
			byte[] res = temp.toString().getBytes();
			try {
				baos.write(res);
			} catch (IOException e) {
				throw new CCPException(e);
			}
		}
	}

	private static void imageContentToUpload(OutputStream out, String imgpath)
			throws CCPException {
		if (imgpath == null) {
			return;
		}
		StringBuilder temp = new StringBuilder();

		temp.append(MP_BOUNDARY).append("\r\n");
		temp.append("Content-Disposition: form-data; name=\"pic\"; filename=\"")
				.append("news_image").append("\"\r\n");
		String filetype = "image/png";
		temp.append("Content-Type: ").append(filetype).append("\r\n\r\n");
		byte[] res = temp.toString().getBytes();
		FileInputStream input = null;
		try {
			out.write(res);
			input = new FileInputStream(imgpath);
			byte[] buffer = new byte[1024 * 50];
			while (true) {
				int count = input.read(buffer);
				if (count == -1) {
					break;
				}
				out.write(buffer, 0, count);
			}
			out.write("\r\n".getBytes());
			out.write(("\r\n" + END_MP_BOUNDARY).getBytes());
		} catch (IOException e) {
			throw new CCPException(e);
		} finally {
			if (null != input) {
				try {
					input.close();
				} catch (IOException e) {
					throw new CCPException(e);
				}
			}
		}
	}

	/**
	 * 
	 * @param out
	 * @param filepath
	 * @throws CCPException
	 */
	private static void fileToUpload(OutputStream out, String filepath)
			throws CCPException {
		if (filepath == null) {
			return;
		}
		StringBuilder temp = new StringBuilder();

		temp.append(MP_BOUNDARY).append("\r\n");

		temp.append(
				"content-disposition: form-data; name=\"file\"; filename=\"")
				.append(filepath).append("\"\r\n");
		temp.append("Content-Type: application/octet-stream; charset=utf-8\r\n\r\n");
		byte[] res = temp.toString().getBytes();
		FileInputStream input = null;
		try {
			out.write(res);
			input = new FileInputStream(filepath);
			byte[] buffer = new byte[1024 * 50];
			while (true) {
				int count = input.read(buffer);
				if (count == -1) {
					break;
				}
				out.write(buffer, 0, count);
			}
			out.write("\r\n".getBytes());
			out.write(("\r\n" + END_MP_BOUNDARY).getBytes());
		} catch (IOException e) {
			throw new CCPException(e);
		} finally {
			if (null != input) {
				try {
					input.close();
				} catch (IOException e) {
					throw new CCPException(e);
				}
			}
		}
	}

	/**
	 * @param response
	 * @return
	 */
	private static String readHttpResponse(HttpResponse response) {
		String result = "";
		HttpEntity entity = response.getEntity();
		InputStream inputStream;
		try {
			inputStream = entity.getContent();
			ByteArrayOutputStream content = new ByteArrayOutputStream();

			Header header = response.getFirstHeader("Content-Encoding");
			if (header != null
					&& header.getValue().toLowerCase().indexOf("gzip") > -1) {
				inputStream = new GZIPInputStream(inputStream);
			}

			int readBytes = 0;
			byte[] sBuffer = new byte[512];
			while ((readBytes = inputStream.read(sBuffer)) != -1) {
				content.write(sBuffer, 0, readBytes);
			}
			result = new String(content.toByteArray(), "UTF-8");
			
			Log4Util.w(Device.TAG, "Response body :\r\n" + result + "\r\n");
			return result;
		} catch (IllegalStateException e) {
		} catch (IOException e) {
		}
		return result;
	}
	
	/**
	 * Generation 11 bit boundary
	 */
	static String getBoundry() {
		StringBuffer _sb = new StringBuffer();
		for (int t = 1; t < 12; t++) {
			long time = System.currentTimeMillis() + t;
			if (time % 3 == 0) {
				_sb.append((char) time % 9);
			} else if (time % 3 == 1) {
				_sb.append((char) (65 + time % 26));
			} else {
				_sb.append((char) (97 + time % 26));
			}
		}
		return _sb.toString();
	}
}
