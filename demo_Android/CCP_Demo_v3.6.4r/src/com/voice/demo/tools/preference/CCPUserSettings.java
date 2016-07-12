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
 */package com.voice.demo.tools.preference;

import android.annotation.SuppressLint;

/**
 * 
 * 
 * @ClassName: CCPUserSettings.java
 * @Description: An enumeration of the search result sort modes.
 * @author Jorstin Chan 
 * @date 2013-12-11
 * @version 3.6
 */
public enum CCPUserSettings implements ObjectStringIdentifier {

	/**
	 * user sub account
	 */
	SUB_ACCOUNT("com.voice.sub_account" , null),
	
	/**
	 * The sub account token.
	 */
	SUB_ACCOUNT_TOKEN("com.voice.sub_account_token" , null),
	
	/**
	 * The user voip account
	 */
	VOIP_ID("com.voice.voip_id" , null),
	
	/**
	 * The voip account token
	 */
	VOIP_TOKEN("com.voice.voip_token" , null);
	
    @SuppressLint("ParserError")
	private String mId;
    private String mValue;

    /**
     * Constructor of <code>CCPUserSettings</code>.
     *
     * @param id The unique identifier of the enumeration
     * @param value the user identifier value
     */
    private CCPUserSettings(String id, String value) {
        this.mId = id;
        this.mValue = value;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getId() {
        return this.mId;
    }

    /**
     * Method that returns the user identifier value
     *
     * @return String the user identifier value
     */
    public String getDefaultValue() {
        return this.mValue;
    }

    /**
     * Method that returns an instance of {@link CCPUserSettings} from its
     * unique identifier.
     *
     * @param id The unique identifier
     * @return CCPUserSettings The user identifier value
     */
    public static CCPUserSettings fromId(String id) {
        CCPUserSettings[] values = values();
        int cc = values.length;
        for (int i = 0; i < cc; i++) {
            if (values[i].mId.compareTo(id) == 0) {
                return values[i];
            }
        }
        return null;
    }

}
