/**
 * 
 */
package com.hisun.phone.core.voice;

import java.io.IOException;
import java.net.Socket;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.Principal;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

/**
 * @author chao
 * 
 */
public class CCPSSLSocketFactory extends org.apache.http.conn.ssl.SSLSocketFactory {
	private SSLContext sslContext = SSLContext.getInstance("TLS");

	CCPSSLSocketFactory(String[] allowedCommonNames) throws KeyManagementException, NoSuchAlgorithmException,
			KeyStoreException, UnrecoverableKeyException {
		super(KeyStore.getInstance(KeyStore.getDefaultType()));
		this.sslContext.init(null, new TrustManager[] { new CCPX509TrustManager(allowedCommonNames) }, null);
	}

	public Socket createSocket(Socket socket, String host, int port,
			boolean autoClose) throws IOException {
		return this.sslContext.getSocketFactory().createSocket(socket, host,
				port, autoClose);
	}

	public Socket createSocket() throws IOException {
		return this.sslContext.getSocketFactory().createSocket();
	}

	private static class CCPX509TrustManager implements X509TrustManager {
		private final String[] allowedCommonNames;
		private final List<X509TrustManager> x509TrustManagers = new ArrayList<X509TrustManager>();

		CCPX509TrustManager(String[] allowedCommonNames) throws NoSuchAlgorithmException, KeyStoreException {
			this.allowedCommonNames = new String[allowedCommonNames.length];
			System.arraycopy(allowedCommonNames, 0, this.allowedCommonNames, 0, allowedCommonNames.length);

			TrustManagerFactory tmFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
			tmFactory.init((KeyStore) null);

			for (TrustManager tm : tmFactory.getTrustManagers()) {
				if ((tm instanceof X509TrustManager)) {
					this.x509TrustManagers.add((X509TrustManager) tm);
				}
			}
		}

		private String parseDN(String dn, String key) {
			String[] parts = dn.split(",");
			if (parts == null) {
				return null;
			}
			for (String part : parts) {
				String[] itemParts = part.split("=", 2);
				if ((itemParts == null) || (itemParts.length < 2))
					continue;
				if (itemParts[0].equals(key)) {
					return itemParts[1];
				}
			}
			return null;
		}

		public void checkClientTrusted(X509Certificate[] chain, String authType)
				throws CertificateException {
			for (X509TrustManager tm : this.x509TrustManagers) {
				try {
					tm.checkClientTrusted(chain, authType);
					return;
				} catch (CertificateException e) {
				}
			}
			throw new CertificateException();
		}

		public void checkServerTrusted(X509Certificate[] chain, String authType)
				throws CertificateException {
			for (X509TrustManager tm : this.x509TrustManagers) {
				try {
					tm.checkServerTrusted(chain, authType);

					Principal subjectDN = chain[0].getSubjectDN();
					String subjectCN = parseDN(subjectDN.getName(), "CN");
					if (subjectCN != null) {
						for (String allowedCN : this.allowedCommonNames) {
							if (subjectCN.equals(allowedCN)) {
								return;
							}
						}
					}
				} catch (CertificateException e) {
				}
			}
			throw new CertificateException();
		}

		@SuppressWarnings("unchecked")
		public X509Certificate[] getAcceptedIssuers() {
			List allIssuers = new ArrayList();
			for (X509TrustManager tm : this.x509TrustManagers) {
				allIssuers.addAll(Arrays.asList(tm.getAcceptedIssuers()));
			}
			return (X509Certificate[]) allIssuers.toArray(new X509Certificate[allIssuers.size()]);
		}
	}

}
