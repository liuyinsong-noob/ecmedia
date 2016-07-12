package com.hisun.phone.core.voice.model;

import android.text.TextUtils;
import java.util.ArrayList;

/**
 * @ClassName: CCPParameters.java
 * @Description: TODO
 * @author Jorstin Chan 
 * @date 2013-12-19
 * @version 3.6
 */
public class CCPParameters {
	
	/**
	 * The HTTP request parameter key
	 */
	private ArrayList<String> mKeys = new ArrayList<String>();
	
	/**
	 * The HTTP request parameter value
	 */
	private ArrayList<String> mValues = new ArrayList<String>();
	
	/**
	 * The http request body Tag
	 */
	private String paramerTagKey = null;

	/**
	 * 
	 * @param key
	 * @param value
	 */
	public void add(String key, String value) {
		if ((!TextUtils.isEmpty(key)) && (!TextUtils.isEmpty(value))) {
			this.mKeys.add(key);
			this.mValues.add(value);
		}
	}

	public void add(String key, int value) {
		this.mKeys.add(key);
		this.mValues.add(String.valueOf(value));
	}

	public void add(String key, long value) {
		this.mKeys.add(key);
		this.mValues.add(String.valueOf(value));
	}

	public void remove(String key) {
		int firstIndex = this.mKeys.indexOf(key);
		if (firstIndex >= 0) {
			this.mKeys.remove(firstIndex);
			this.mValues.remove(firstIndex);
		}
	}
	
	public boolean validate() {
		if((!mKeys.isEmpty() && !mValues.isEmpty()) && (mKeys.size() == mValues.size())) {
			return true;
		}
		return false;
	}

	public void remove(int i) {
		if (i < this.mKeys.size()) {
			this.mKeys.remove(i);
			this.mValues.remove(i);
		}
	}

	private int getLocation(String key) {
		if (this.mKeys.contains(key)) {
			return this.mKeys.indexOf(key);
		}
		return -1;
	}

	public String getKey(int location) {
		if ((location >= 0) && (location < this.mKeys.size())) {
			return (String) this.mKeys.get(location);
		}
		return "";
	}

	public String getValue(String key) {
		int index = getLocation(key);
		if ((index >= 0) && (index < this.mKeys.size())) {
			return (String) this.mValues.get(index);
		}
		return null;
	}

	public String getValue(int location) {
		if ((location >= 0) && (location < this.mKeys.size())) {
			String rlt = (String) this.mValues.get(location);
			return rlt;
		}
		return null;
	}

	public int size() {
		return this.mKeys.size();
	}

	public void addAll(CCPParameters parameters) {
		for (int i = 0; i < parameters.size(); i++)
			add(parameters.getKey(i), parameters.getValue(i));
	}
	

	public String getParamerTagKey() {
		return paramerTagKey;
	}

	public void setParamerTagKey(String paramerTagKey) {
		this.paramerTagKey = paramerTagKey;
	}

	public void clear() {
		this.mKeys.clear();
		this.mValues.clear();
	}
}