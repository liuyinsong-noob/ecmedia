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
 */package com.voice.demo.tools;
/**
 * 
 * @author Jorstin Chan
 * @version 3.3
 */
public class SDKVersion {
	
	private String version;      // The version number
	private String platform;     // platform version of SDK (Android、 Windows、 iOS、 Mac OS、 Linux)
	private String ARMVersion;   // ARM (arm、 armv7、 armv5)
	private boolean audioSwitch; // audio switch (voice=false、 voice=true)
	private boolean videoSwitch; // video switch (video=false、 video=true)
	private String compileDate;  // compile date ("MM DD YYYY"  如"Jan 19 2013" "hh:mm:ss"    如”08:30:23”)

	public String getVersion() {
		return version;
	}

	public void setVersion(String version) {
		this.version = version;
	}

	public String getPlatform() {
		return platform;
	}

	public void setPlatform(String platform) {
		this.platform = platform;
	}

	public String getARMVersion() {
		return ARMVersion;
	}

	public void setARMVersion(String aRMVersion) {
		ARMVersion = aRMVersion;
	}

	public boolean isAudioSwitch() {
		return audioSwitch;
	}

	public void setAudioSwitch(boolean audioSwitch) {
		this.audioSwitch = audioSwitch;
	}

	public boolean isVideoSwitch() {
		return videoSwitch;
	}

	public void setVideoSwitch(boolean videoSwitch) {
		this.videoSwitch = videoSwitch;
	}

	public String getCompileDate() {
		return compileDate;
	}

	public void setCompileDate(String compileDate) {
		this.compileDate = compileDate;
	}

	@Override
	public String toString() {
		return "SDKVersion [version=" + version + ", platform=" + platform
				+ ", ARMVersion=" + ARMVersion + ", audioSwitch=" + audioSwitch
				+ ", videoSwitch=" + videoSwitch + ", compileDate="
				+ compileDate + "]";
	}

	
}
