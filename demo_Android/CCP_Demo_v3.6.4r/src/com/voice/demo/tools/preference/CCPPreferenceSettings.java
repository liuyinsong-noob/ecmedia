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
package com.voice.demo.tools.preference;


/**
 * 
 * @ClassName: CCPPreferenceSettings.java
 * @Description: The enumeration of settings of CCP application.
 * @author Jorstin Chan 
 * @date 2013-12-6
 * @version 3.6
 */
public enum CCPPreferenceSettings {

	
	/**
     * Whether is the first use of the application
     * 
     */
    SETTINGS_FIRST_USE("com_voice_demo_use", Boolean.TRUE),  
    
	/**
	 * The user name for login CCP Demo of Experience Mode
	 * 
	 */
	SETTING_USERNAME_MAIL("com_voice_demo_mail" , new String("")),
	
	/**
	 * The user pass for login CCP Demo of Experience Mode
	 * 
	 */
	SETTING_USER_PASSWORD("com_voice_demo_userpas" , new String("")),
	
	/**
	 * The join groups of IM group
	 */
	SETTING_JOIN_GROUP_SIZE("com_voice_demo_joingroups" , 0),
	
	/**
	 * The public groups of IM group
	 */
	SETTING_PUB_GROUP_SIZE("com_voice_demo_pubgroups" , 0),
	
	/**
	 * The netcheck
	 */
	SETTING_NETCHECK("com_voice_demo_netcheck" , Boolean.TRUE),
	/**
	 * The upload voice mode 
	 */
	SETTING_VOICE_ISCHUNKED("com_voice_demo_voice_ischunked" , Boolean.TRUE),
	
	/**
	 * The user phone number
	 */
	SETTING_PHONE_NUMBER("com_voice_demo_phonenumber" , new String("")),
	
	/**
	 * The client is opened for the first time ,then show the instructions view of Video Conference.
	 */
	SETTING_SHOW_INSTRUCTIONS_VIEW("com_voice_demo_instructions_view" , Boolean.TRUE),
	
	/**
	 * The client is opened for the first time ,then show the welcome view.
	 */
	SETTING_SHOW_WELCOME_VIEW("com_voice_demo_welcome_vice" , Boolean.TRUE),
	
	/**
	 * Automatic gain control(自动增益控制)
	 */
	SETTING_AUTO_MANAGE("com_voice_demo_automanage" , Boolean.FALSE),
	
	/**
	 * Automatic gain control default value.
	 */
	SETTING_AUTO_MANAGE_VALUE("com_voice_demo_automanage_value" , Integer.valueOf(3)),
	
	/**
	 * Echo cancellation(回音消除)
	 */
	SETTING_ECHOCANCEL("com_voice_demo_echocancelled" , Boolean.TRUE),
	
	/**
	 * Echo cancellation default value.
	 */
	SETTING_ECHOCANCEL_VALUE("com_voice_demo_echocancelled_value" , Integer.valueOf(4)),
	
	/**
	 *  Silence suppression(静音抑制)
	 */
	SETTING_SILENCERESTRAIN("com_voice_demo_silencerestrain" , Boolean.TRUE),
	
	/**
	 * Silence suppression default value.
	 */
	SETTING_SILENCERESTRAIN_VALUE("com_voice_demo_silencerestrain_value" , Integer.valueOf(6)),
	
	/**
	 * Video control(视频码流控制)
	 */
	SETTING_VIDEO_STREAM_CONTROL("com_voice_demo_videostream" , Boolean.TRUE),
	
	/**
	 *Video control default value. 
	 */
	SETTING_VIDEO_STREAM_CONTROL_VALUE("com_voice_demo_videostream_value" , Integer.valueOf(150)),
	
	SETTING_CODE_VALUE("com_voice_demo_supcodec_value" , Integer.valueOf(-1)),
	SETTING_CODE("com_voice_demo_supcodec" , Boolean.TRUE),

	SETTING_VIDEO_CALL_RESOLUTION("com_voice_demo_video_call_resolution" , Integer.valueOf(-1)),
	/**
	 * P2P 
	 */
	SETTING_P2P("com_voice_demo_p2p" , Boolean.FALSE),
	
	/**
	 * p2p server deault ip 
	 */
	SETTING_P2P_VALUE("com_voice_demo_p2p_value" , new String("42.121.15.99:3478")),
	
	/**
	 * set larger stream (P2P通话码流设置)
	 */
	SETTING_P2P_STARTBITRATE("com_voice_demo_p2p_startbitrate" ,  Boolean.FALSE),
	
	/**
	 * p2p startbutrate default value.
	 */
	SETTING_P2P_STARTBITRATE_VALUE("com_voice_demo_p2p_startbitrate_value" ,  Integer.valueOf("330")),
	
	
	/**
	 * shielded video decoding process of mosaic(P2P视频通话马赛克抑制)
	 */
	SETTING_P2P_VIDEO_MOSAIC("com_voice_demo_p2p_video_mosaic" , Boolean.FALSE),
	
	/**
	 * audio speaker on or false
	 */
	SETTING_HANDSET("com_voice_demo_handset" , Boolean.FALSE),
	SETTINGS_VIDEO_CONF_FIRST ("com_voice_demo_video_conf_first" , Boolean.FALSE),
    
    /**
     * Calls Record on or false
     * @version 3.6
     */
	SETTING_CALL_RECORDING("com_voice_demo_calls_recording" , Boolean.FALSE);
	
    private final String mId;
    private final Object mDefaultValue;

    /**
     * Constructor of <code>CCPPreferenceSettings</code>.
     *
     * @param id The unique identifier of the setting
     * @param defaultValue The default value of the setting
     */
    private CCPPreferenceSettings(String id, Object defaultValue) {
        this.mId = id;
        this.mDefaultValue = defaultValue;
    }

    /**
     * Method that returns the unique identifier of the setting.
     * @return the mId
     */
    public String getId() {
        return this.mId;
    }

    /**
     * Method that returns the default value of the setting.
     *
     * @return Object The default value of the setting
     */
    public Object getDefaultValue() {
        return this.mDefaultValue;
    }

    /**
     * Method that returns an instance of {@link CCPPreferenceSettings} from its.
     * unique identifier
     *
     * @param id The unique identifier
     * @return CCPPreferenceSettings The navigation sort mode
     */
    public static CCPPreferenceSettings fromId(String id) {
    	CCPPreferenceSettings[] values = values();
        int cc = values.length;
        for (int i = 0; i < cc; i++) {
            if (values[i].mId == id) {
                return values[i];
            }
        }
        return null;
    }
}
