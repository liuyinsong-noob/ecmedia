package com.hisun.phone.core.voice.model.setup;


import java.util.ArrayList;

import android.text.TextUtils;

import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.util.Log4Util;
import com.hisun.phone.core.voice.util.VoiceUtil;

public class SoftSwitch extends Response {
	

	/**
	 * 
	 */
	private static final long serialVersionUID = -7103314886132705486L;
	
	private ArrayList<SoftSwitch.Clpss> mClpsses = new ArrayList<SoftSwitch.Clpss>();
	
	private String P2PServerPort;
	
	/**
	 * 
	 */
	private String control;
	
	/**
	 * 支持IP路由组的功能
	 */
	private String nwgid;
	
	/**
	 * 
	 * @return p2p server ip and port
	 */
	public String getP2PServerPort() {
		return P2PServerPort;
	}

	public void setP2PServerPort(String p2pServerPort) {
		P2PServerPort = p2pServerPort;
	}

	public void addSoftClpsses(SoftSwitch.Clpss clpss) {
		
		if(mClpsses == null) {
			mClpsses = new ArrayList<SoftSwitch.Clpss>();
		}
		
		mClpsses.add(clpss);
	}

	/**
	 * 
	 */
	public void clearSoftClpsses() {
		if(mClpsses != null) {
			mClpsses.clear();
		}
	}
	
	public ArrayList<SoftSwitch.Clpss> getSoftClpss() {
		if(mClpsses == null ) {
			mClpsses = new ArrayList<SoftSwitch.Clpss>();
		}
		return mClpsses;
	}


	public String getControl() {
		return control;
	}

	public void setNetworkGroupId(String nwgid) {
		this.nwgid = nwgid;
	}

	public String getNetworkGroupId() {
		return nwgid;
	}

	public void setControl(String control) {
		this.control = control;
	}

	/**
	 * 
	 */
	public String toXMLBody() {
		StringBuffer sb = new StringBuffer(VoiceUtil.XMLHeader + "\r\n");
		sb.append("<Response>\r\n");
		sb.append("\t<statusCode>").append(statusCode).append("</statusCode>\r\n");
		if(getSoftClpss() != null && !getSoftClpss().isEmpty()) {
			sb.append("\t<Switch>\r\n");
			for(Clpss clpss : getSoftClpss()) {
				if(clpss == null) {
					continue;
				}
				sb.append("\t\t<clpss>\r\n");
				sb.append("\t\t\t<ip>").append(clpss.getIp()).append("</ip>\r\n");
				sb.append("\t\t\t<port>").append(clpss.getPort()).append("</port>\r\n");
				sb.append("\t\t\t</clpss>\r\n");
			}
			if(!TextUtils.isEmpty(getControl())) {
				sb.append("\t\t<control>").append(getControl()).append("</control>\r\n");
			}
			if(!TextUtils.isEmpty(getP2PServerPort())) {
				sb.append("\t\t<p2p>").append(getP2PServerPort()).append("</p2p>\r\n");
			}
			sb.append("\t</Switch>\r\n");
		}
		sb.append("</Response>\r\n");
		return sb.toString();
	}

	public static class Clpss {
		
		/**
		 * ip
		 */
		public String ip;
		
		/**
		 * port : 8881
		 */
		public String port;
		
		/**
		 * @return
		 */
		public String getIp() {
			return ip;
		}
		public void setIp(String ip) {
			this.ip = ip;
		}
		
		public int getPort() {
			try {
				return Integer.parseInt(port);
			} catch (Exception e) {
				return 0;
			}
		}

		public void setPort(String port) {
			this.port = port;
		}
		
		
	}
}
