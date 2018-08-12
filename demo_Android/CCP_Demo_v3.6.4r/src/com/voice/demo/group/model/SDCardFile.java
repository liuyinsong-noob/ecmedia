/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.group.model;

import android.graphics.drawable.Drawable;

public class SDCardFile implements  Comparable<SDCardFile>{
	
	public String fileName;
	public Drawable drawable;
	
	public boolean isDirectory;
	
	
	@Override
	public int compareTo(SDCardFile another) {
		return this.fileName.compareTo(another.fileName);
	}
	
	 /**
     * Method that returns of the object is hidden object.
     *
     * @return boolean If the object is hidden object
     */
    public boolean isHidden() {
        return this.fileName.startsWith("."); //$NON-NLS-1$
    }

	public boolean isDirectory() {
		return isDirectory;
	}

    
}
