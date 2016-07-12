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

import java.io.InvalidClassException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.CCPApplication;
import com.voice.demo.ui.CCPHelper;

/**
 * 
 * @ClassName: CcpPreferences 
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-3
 *
 */
public class CcpPreferences {
	
	/**
	 * The name of the CCP Demo settings file.
	 */
	public static final String CCP_DEMO_PREFERENCE = "com.voice.demo.preference";
	
	/**
	 * The user config.
	 */
	public static final String CCP_USER_CONFIG = "com.voice.demo_userconfig";
	
	/**
     * Constructor of <code>CcpPreferences</code>.
     */
	private CcpPreferences() {
		super();
	}
	
	 /**
     * Method that initializes the defaults preferences of the application.
     */
    public static void loadDefaults() {
        //Sets the default preferences if no value is set yet
        try {
            Map<CCPPreferenceSettings, Object> defaultPrefs =
                    new HashMap<CCPPreferenceSettings, Object>();
            CCPPreferenceSettings[] values = CCPPreferenceSettings.values();
            int cc = values.length;
            for (int i = 0; i < cc; i++) {
                defaultPrefs.put(values[i], values[i].getDefaultValue());
            }
            savePreferences(defaultPrefs, false, true);
        } catch (Exception ex) {
        	ex.printStackTrace();
            Log4Util.e(CCPHelper.DEMO_TAG, "Save default settings fails"); 
        }
    }
    

	/**
     * Method that returns the shared preferences of the application.
     *
     * @return SharedPreferences The shared preferences of the application
     * @hide
     */
    public static SharedPreferences getSharedPreferences() {
        return CCPApplication.getInstance().getSharedPreferences(
        		CCP_DEMO_PREFERENCE, Context.MODE_PRIVATE);
    }
    
    /**
	 * To obtain the system preferences to save the file to edit the object
	 * @return
	 */
	public static Editor getSharedPreferencesEditor() {
		SharedPreferences cCPreferences = getSharedPreferences();
		Editor edit = cCPreferences.edit();
		edit.remove("");
		return edit;
	}
	
	  /**
     * Method that saves a preference.
     *
     * @param pref The preference identifier
     * @param value The value of the preference
     * @param applied If the preference was applied
     * @throws InvalidClassException If the value of the preference is not of the
     * type of the preference
     */
    public static void savePreference(CCPPreferenceSettings pref, Object value, boolean applied)
            throws InvalidClassException {
        Map<CCPPreferenceSettings, Object> prefs =
                new HashMap<CCPPreferenceSettings, Object>();
        prefs.put(pref, value);
        savePreferences(prefs, applied);
    }
    
    /**
     * Method that saves the preferences passed as argument.
     *
     * @param prefs The preferences to be saved
     * @param applied If the preference was applied
     * @throws InvalidClassException If the value of a preference is not of the
     * type of the preference
     */
    public static void savePreferences(Map<CCPPreferenceSettings, Object> prefs, boolean applied)
            throws InvalidClassException {
        savePreferences(prefs, true, applied);
    }

    /**
     * Method that saves the preferences passed as argument.
     *
     * @param prefs The preferences to be saved
     * @param noSaveIfExists No saves if the preference if has a value
     * @param applied If the preference was applied
     * @throws InvalidClassException If the value of a preference is not of the
     * type of the preference
     */
    @SuppressLint("NewApi")
	@SuppressWarnings("unchecked")
    private static void savePreferences(
            Map<CCPPreferenceSettings, Object> prefs, boolean noSaveIfExists, boolean applied)
            throws InvalidClassException {
        //Get the preferences editor
        SharedPreferences sp = getSharedPreferences();
        Editor editor = sp.edit();

        //Save all settings
        Iterator<CCPPreferenceSettings> it = prefs.keySet().iterator();
        while (it.hasNext()) {
        	CCPPreferenceSettings pref = it.next();
            if (!noSaveIfExists && sp.contains(pref.getId())) {
                //The preference already has a value
                continue;
            }

            //Known and valid types
            Object value = prefs.get(pref);
            if(value == null ) {
            	return;
            }
            if (value instanceof Boolean && pref.getDefaultValue() instanceof Boolean) {
                editor.putBoolean(pref.getId(), ((Boolean)value).booleanValue());
            } else if (value instanceof String && pref.getDefaultValue() instanceof String) {
                editor.putString(pref.getId(), (String)value);
            }else if (value instanceof Integer && pref.getDefaultValue() instanceof Integer) {
                editor.putInt(pref.getId(), (Integer)value);
            } else if (value instanceof Set && pref.getDefaultValue() instanceof Set) {
                editor.putStringSet(pref.getId(), (Set<String>)value);
            } else if (value instanceof ObjectStringIdentifier
                    && pref.getDefaultValue() instanceof ObjectStringIdentifier) {
                editor.putString(pref.getId(), ((ObjectStringIdentifier)value).getId());
            }   else {
                //The object is not of the appropriate type
                String msg = String.format(
                                    "%s: %s",  
                                    pref.getId(),
                                    value.getClass().getName());
                Log4Util.e(CCPHelper.DEMO_TAG, String.format(
                                "Configuration error. InvalidClassException: %s",  
                                msg));
                throw new InvalidClassException(msg);
            }

        }

        //Commit settings
        editor.commit();

    }
}
