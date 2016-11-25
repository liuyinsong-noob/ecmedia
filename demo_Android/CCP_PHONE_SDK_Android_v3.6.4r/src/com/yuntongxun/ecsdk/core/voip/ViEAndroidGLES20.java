/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package com.yuntongxun.ecsdk.core.voip;

import android.view.SurfaceView;

import com.hisun.phone.core.voice.DeviceImpl;
import com.hisun.phone.core.voice.util.Log4Util;
import com.yuntongxun.ecsdk.voip.video.ECOpenGlRender;
import com.yuntongxun.ecsdk.voip.video.ECOpenGlView;

public class ViEAndroidGLES20 {
    private static String TAG = "WEBRTC-GLES20";
    private static final boolean DEBUG = true;
    // True if onSurfaceCreated has been called.
    private ECOpenGlView glView = null;
    private ECOpenGlRender glRender = null;
    private byte[] videoData = null;

    public static boolean UseOpenGL2(String account) {
        if(DEBUG)
            Log4Util.w(TAG, "ViEAndroidGLES20 UseOpenGL2 account " + account);
        SurfaceView renderWindow = DeviceImpl.getVideoView(account);
        return ECOpenGlView.UseOpenGL2(renderWindow);
    }

    public ViEAndroidGLES20(String account) {
        if(DEBUG)
            Log4Util.w(TAG, "ViEAndroidGLES20 account " + account);
        glView = (ECOpenGlView)DeviceImpl.getVideoView(account);
        glRender = glView.getRenderer();
    }

    public void SetCoordinates(int zOrder, float left, float top, float right, float bottom) {
        if(DEBUG)
            Log4Util.w(TAG, "ViEAndroidGLES20 SetCoordinates zOrder:"+zOrder+" left:"+left+" top:"
                    +top+" right:"+right+" bottom:"+bottom);
    }

    public void ReDraw(int w, int h, byte[] data, int datalen) {
        if(DEBUG)
            Log4Util.w(TAG, "ViEAndroidGLES20 ReDraw w:"+w+" h:"+h + " len:"+datalen);

        if(videoData==null || videoData.length < datalen)
            videoData = new byte[datalen];
        System.arraycopy(data, 0, videoData,0,datalen);
        glRender.renderFrame(videoData, w, h, datalen);
    }
}




