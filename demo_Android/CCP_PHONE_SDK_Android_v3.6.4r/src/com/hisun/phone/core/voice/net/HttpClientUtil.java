package com.hisun.phone.core.voice.net;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.io.Reader;
import java.net.HttpURLConnection;
import java.net.URL;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.SSLSession;

import android.os.Build;
import android.text.TextUtils;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.RecordProduct;
import com.hisun.phone.core.voice.model.RecordProduct.RecordProductType;
import com.hisun.phone.core.voice.multimedia.CCPAudioRecorder;
import com.hisun.phone.core.voice.multimedia.RecordBlockingQueue;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.SdkErrorCode;


public class HttpClientUtil {
	private HttpClientUtil() {

	}

	public static String postRequestUploadChunk(String reqURL) {
		HttpURLConnection httpURLConnection = null;
		DataOutputStream out = null; 
		InputStream in = null; 
		int httpStatusCode = 0; 
		try {
			URL sendUrl = new URL(reqURL);
			httpURLConnection = (HttpURLConnection) sendUrl.openConnection();
			httpURLConnection.setRequestMethod("POST");
			httpURLConnection.setRequestProperty("Content-Type", "application/octet-stream");
			httpURLConnection.setRequestProperty("Transfer-Encoding","chunked");
			httpURLConnection.setRequestProperty("Accept", "application/xml");
			//httpURLConnection.setRequestProperty("Connection", "Keep-Alive");
			if (Build.VERSION.SDK_INT > 13) {
				httpURLConnection.setRequestProperty("Connection", "close");
			}
			// Work around pre-Froyo bugs in HTTP connection reuse.
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.FROYO) {
				System.setProperty("http.keepAlive", "false");

			}
			httpURLConnection.setDoOutput(true); 
			httpURLConnection.setConnectTimeout(60000); 
			httpURLConnection.setReadTimeout(60000);
			httpURLConnection.setChunkedStreamingMode(650);
			out = new DataOutputStream(httpURLConnection.getOutputStream());

			RecordBlockingQueue instance = RecordBlockingQueue.getInstance();
			RecordProduct product = instance.consume();
			while (product.productType != RecordProductType.ProductEnd) {
				if(product.productType == RecordProductType.ProductData){
					out.write(product.data, 0, product.data.length);
					out.flush();
				}
				product = instance.consume();
			}
			out.close();
			Log4Util.d(Device.TAG ,  "over");
			
			httpStatusCode = httpURLConnection.getResponseCode();
			Log4Util.d(Device.TAG ,  "[HttpClientUtil - postRequestUploadChunk] httpStatusCode : " + httpStatusCode + " , body:" + httpURLConnection.getResponseMessage());
			
			//int statusCode = httpURLConnection.getResponseCode();
			if (httpStatusCode == java.net.HttpURLConnection.HTTP_OK) {
				in = httpURLConnection.getInputStream();
				String stringFromInputStream =  stringFromInputStream(in);
				in.close();
				return stringFromInputStream;
				
			} else {
				throw new Exception("Got error code " + in + " from server.");
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			Log4Util.d(Device.TAG ,  "[HttpClientUtil - postRequestUploadChunk]  httpURLConnection disconnect");
			if (httpURLConnection != null) {
				httpURLConnection.disconnect();
				httpURLConnection = null;
			}
		}
		return null;
	}

	public static int postRequestDownload(String url , String svaePath) {
		HttpURLConnection httpsURLConnection = null;
		InputStream in = null; 
		int httpStatusCode = 0; 
		try {
			URL sendUrl = new URL(url);
			httpsURLConnection = (HttpURLConnection) sendUrl.openConnection();
			httpsURLConnection.setRequestMethod("GET");
			httpsURLConnection.setRequestProperty("Accept", "application/octet-stream");
			httpsURLConnection.setRequestProperty("Connection", "Keep-Alive");
			httpsURLConnection.setDoOutput(false); 
			httpsURLConnection.setDoInput(true); 
			httpsURLConnection.setConnectTimeout(60000); 
			
			httpsURLConnection.setReadTimeout(60000);
			httpsURLConnection.connect();

			httpStatusCode = httpsURLConnection.getResponseCode();
			
			Log4Util.d(Device.TAG ,  "[HttpClientUtil - postRequestDownload] : " + httpStatusCode + " , body:" + httpsURLConnection.getResponseMessage());
			
			//int statusCode = httpsURLConnection.getResponseCode();
			if (httpStatusCode == java.net.HttpURLConnection.HTTP_OK) {
				in = httpsURLConnection.getInputStream();
				if(_saveFromInputStream(in , svaePath)) {
					httpStatusCode = 0;
				} else {
					httpStatusCode = SdkErrorCode.SDK_AMR_CANCLE;
				}
				//return saveFromInputStream(in , svaePath);
				
			} else if (httpStatusCode == java.net.HttpURLConnection.HTTP_NOT_FOUND) {
				return SdkErrorCode.SDK_FILE_NOTEXIST;
			}  else {
				Log4Util.i(Device.TAG ,  "[HttpClientUtil - postRequestDownload] Got error code " + in + " from server.");
				//throw new Exception("Got error code " + in + " from server.");
			}
		} catch (IOException e) {
			e.printStackTrace();
			httpStatusCode = SdkErrorCode.SDK_WRITE_FAILED;
		}catch (Exception e) {
			e.printStackTrace();
			new File(svaePath).deleteOnExit();
			Log4Util.d(Device.TAG ,  "[HttpClientUtil - postRequestDownload] : Don't read data flow normal end marker, notice of cancellation " +
					", and delete local voice file .");
			httpStatusCode = SdkErrorCode.SDK_HTTP_ERROR;
		} finally {
			if (httpsURLConnection != null) {
				httpsURLConnection.disconnect();
				httpsURLConnection = null;
			}
		}
		
		return httpStatusCode;
	}
	
	/**
	 * Upload file
	 * @param reqURL
	 * @param fileName
	 * @return
	 */
	public static String postRequestUploadFileChunk(String reqURL , String fileName) {
		HttpURLConnection httpURLConnection = null;
		DataOutputStream out = null; 
		InputStream in = null; 
		int httpStatusCode = 0; 
		try {
			URL sendUrl = new URL(reqURL);
			httpURLConnection = (HttpURLConnection) sendUrl.openConnection();
			httpURLConnection.setRequestMethod("POST");
			httpURLConnection.setRequestProperty("Content-Type", "application/octet-stream");
			httpURLConnection.setRequestProperty("Transfer-Encoding","chunked");
			httpURLConnection.setRequestProperty("Accept", "application/xml");
			//httpURLConnection.setRequestProperty("Connection", "Keep-Alive");
			if (Build.VERSION.SDK_INT > 13) {
				httpURLConnection.setRequestProperty("Connection", "close");
			}
			
			// Work around pre-Froyo bugs in HTTP connection reuse.
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.FROYO) {
				System.setProperty("http.keepAlive", "false");

			}
			httpURLConnection.setDoOutput(true); 
			httpURLConnection.setConnectTimeout(60000); 
			httpURLConnection.setReadTimeout(60000); 
			httpURLConnection.setChunkedStreamingMode(650);
			out = new DataOutputStream(httpURLConnection.getOutputStream());

			FileInputStream fis = new FileInputStream(fileName);
			byte[] buffer = new byte[1024 * HttpManager.SKIP_CACHE_NUM]; // 1k
			int len = 0;
			
			// Read the contents of a folder, and written to the OutputStream object
			int size = 0;
			while ((len = fis.read(buffer)) != -1){
				out.write(buffer, 0, len);
				size += len;
				Log4Util.d(Device.TAG,
						"[HttpClientUtil - postRequestUploadFileChunk] The total sent files :  "
								+ size + " bytes. ");
			}
			
			// Upload file end, close the input and output streams
			fis.close();
			out.close();
			Log4Util.d(
					Device.TAG,
					"[HttpClientUtil - postRequestUploadFileChunk] Upload file end, close the input and output streams .");
			
			httpStatusCode = httpURLConnection.getResponseCode();
			Log4Util.d(Device.TAG ,  "[HttpClientUtil - postRequestUploadFileChunk] httpStatusCode : " 
					+ httpStatusCode + " , body:" + httpURLConnection.getResponseMessage());
			
			//int statusCode = httpURLConnection.getResponseCode();
			if (httpStatusCode == java.net.HttpURLConnection.HTTP_OK) {
				in = httpURLConnection.getInputStream();
				String stringFromInputStream =  stringFromInputStream(in);
				in.close();
				
				// Read the server returned to upload results information, and then close the input stream
				Log4Util.d(
						Device.TAG,
						"[HttpClientUtil - postRequestUploadFileChunk] Read the server returned to upload results information"
								+ ", and then close the input stream .");
				return stringFromInputStream;
				
			} else {
				throw new Exception("Got error code " + in + " from server.");
			}
		} catch (Exception e) {
			e.printStackTrace();
			
		} finally {
			Log4Util.d(Device.TAG,
					"[HttpClientUtil - postRequestUploadFileChunk] httpURLConnection disconnect");
			
			if (httpURLConnection != null) {
				httpURLConnection.disconnect();
				httpURLConnection = null;
			}
		}
		return null;
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
	
	/**
	 * 
	 * @param content
	 * @param fileName
	 * @return
	 * @throws IOException
	 */
	static boolean _saveFromInputStream(InputStream content,
			String fileName) throws IOException {
		
		RandomAccessFile markRead = null;
		try {
			markRead = new RandomAccessFile(fileName , "rw");
			
			byte[] b = new byte[1024];
			int len = 0;
			int totalLength = 0;
			while ((len = content.read(b)) != -1) {
				totalLength += len;
				markRead.write(b, 0, len);
				Log4Util.d(Device.TAG,
						"[HttpClientUtil - postRequestDownload] : read data byte size :"
								+ len);
			}
			
			byte[] mark = new byte[12]; //声明一个刚好够存放文件流的字节数据
			if(totalLength >= 12) {
				markRead.seek(totalLength - 12);
				markRead.read(mark);
			}
			String markStr = new String(mark);
			
			if(isAmrMark(markStr)) {
				Log4Util.d(Device.TAG,
						"[HttpClientUtil - postRequestDownload] : read data byte over , mark is : "
								+ markStr.substring(1, markStr.length()));
				return true;
			}
			
			new File(fileName).deleteOnExit();
			Log4Util.d(
					Device.TAG,
					"[HttpClientUtil - postRequestDownload] : read amr marke error, mark :  " + markStr 
							+ ", and delete local voice file .");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if(markRead != null) {
				markRead.close();
				markRead = null;
			}
		}

		return false;
	}
	
	static boolean hasStopMark(byte[] toCompare) {
		
		if(toCompare == null || toCompare.length != 11) {
			return false;
		}
		
		for(int i = 0 ; i < 11 ; i++) {
			if(CCPAudioRecorder.AMR_STOP[i] != toCompare[i]) {
				return false;
			}
		}
		
		return true;
	}
	
	/**
	 * @param str
	 * @return
	 */
	static boolean isAmrMark(String str) {
		
		if(TextUtils.isEmpty(str)) {
			return false;
		}
		
		if (str.endsWith(new String(CCPAudioRecorder.AMR_STOP))) {
			return true;
		}

		return false;
	}
	
	@Deprecated
	static boolean saveFromInputStream(InputStream content,
			String fileName) throws IOException {
		FileOutputStream out = new FileOutputStream(fileName);

		byte[] b = new byte[1024];
		int len = 0;
		int lastLength = 0;
		while ((len = content.read(b)) != -1) {
			lastLength = len;
			out.write(b, 0, len);
			Log4Util.d(Device.TAG,
					"[HttpClientUtil - postRequestDownload] : read data byte size :"
							+ len);
		}
		
		byte[] stop = new byte[11];
		if(lastLength >= 11) {
			System.arraycopy(b, lastLength - 11, stop, 0, 11);
			
			if(hasStopMark(stop)) {
				String amrStop = new String(stop);
				Log4Util.d(Device.TAG,
						"[HttpClientUtil - postRequestDownload] : read data byte over , mark is : "
								+ amrStop);
				return true;
			} else {
				Log4Util.d(Device.TAG,
						"[HttpClientUtil - postRequestDownload] : read data byte over , mark is : "
								+ new String(stop));
			}
		} else
		
		new File(fileName).deleteOnExit();
		Log4Util.d(
				Device.TAG,
				"[HttpClientUtil - postRequestDownload] : Don't read data flow normal end marker, notice of cancellation "
						+ ", and delete local voice file .");
		return false;
	}
}