package com.hisun.phone.core.voice.net;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
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

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.HttpVersion;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.conn.ClientConnectionManager;
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

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;

@Deprecated
public final class HttpHelper {
	private static final String TAG = "HttpHelper";
	public static final int SKIP_CACHE_NUM = 128;
	private static HttpClient httpClient;
	private static int sslPort = 0;
	
	private static void ensureHttpClient() {
		///if (httpClient  == null) {
			httpClient = newHttpClient();
		//}
	}

	static HttpClient newHttpClient() {
		try {
			KeyStore trustStore = KeyStore.getInstance(KeyStore.getDefaultType());
			trustStore.load(null, null);

			SSLSocketFactory sf = new SSLSocketFactoryEx(trustStore);
			sf.setHostnameVerifier(SSLSocketFactory.ALLOW_ALL_HOSTNAME_VERIFIER);

			HttpParams params = new BasicHttpParams();
			HttpConnectionParams.setConnectionTimeout(params, 45000);
			HttpConnectionParams.setSoTimeout(params, 30000);
			HttpProtocolParams.setVersion(params, HttpVersion.HTTP_1_1);
			HttpProtocolParams.setContentCharset(params, HTTP.UTF_8);

			SchemeRegistry registry = new SchemeRegistry();
			registry.register(new Scheme("http", PlainSocketFactory.getSocketFactory(), 80));
			registry.register(new Scheme("https", sf, (sslPort <= 0 ? 8883 : sslPort)));

			ClientConnectionManager ccm = new ThreadSafeClientConnManager(params, registry);
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
	
	@Deprecated
	public static String httpGet(String url, HashMap<String, String> headers)
			throws Exception {
		ensureHttpClient();

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

	@Deprecated
	public static String httpPost(final String url, final HashMap<String, String> headers,
			final String requestBody) throws Exception {
		ensureHttpClient();

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
	
	@Deprecated
	public static String httpPost(final String url) throws Exception {
		ensureHttpClient();
		
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
	
	@Deprecated
	public static String httpUploadFile(String url, HashMap<String, String> headers,
			byte[] requestBody) throws Exception {
		ensureHttpClient();

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
		ensureHttpClient();

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
		ensureHttpClient();
		
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
	
	@Deprecated
	public static String httpPut(String url, HashMap<String, String> headers,
			String requestBody) throws Exception {
		ensureHttpClient();

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

}
