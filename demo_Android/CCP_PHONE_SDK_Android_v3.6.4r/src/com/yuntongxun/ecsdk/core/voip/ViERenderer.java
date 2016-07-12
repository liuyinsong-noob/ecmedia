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

import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ViERenderer {

	private static class LocalHolder implements SurfaceHolder.Callback{
		private SurfaceHolder holder = null;
		
		private boolean isHolderReady = false;;
		
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width,
				int height) {

		}

		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			this.holder = holder;
			isHolderReady = true;
		}

		@Override
		public void surfaceDestroyed(SurfaceHolder holder) {
			this.holder = null;
			isHolderReady = false;
		}
		
	}
    // View used for local rendering that Cameras can use for Video Overlay.
    private static SurfaceHolder g_localRenderer;
    private static LocalHolder g_localHolder;
    
    public static SurfaceView CreateRenderer(Context context) {
        return  CreateRenderer(context,false);
    }

    public static SurfaceView CreateRenderer(Context context,
            boolean useOpenGLES2) {
        if(useOpenGLES2 == true && ViEAndroidGLES20.IsSupported(context))
            return new ViEAndroidGLES20(context);
        else
            return new SurfaceView(context);
    }

    // Creates a SurfaceView to be used by Android Camera
    // service to display a local preview.
    // This needs to be used on Android prior to version 2.1
    // in order to run the camera.
    // Call this function before ViECapture::StartCapture.
    // The created view needs to be added to a visible layout
    // after a camera has been allocated
    // (with the call ViECapture::AllocateCaptureDevice).
    // IE.
    // CreateLocalRenderer
    // ViECapture::AllocateCaptureDevice
    // LinearLayout.addview
    // ViECapture::StartCapture
    public static SurfaceView CreateLocalRenderer(Context context) {
        SurfaceView localRender = new SurfaceView(context);
        g_localRenderer = localRender.getHolder();
        g_localHolder = new LocalHolder();
        g_localRenderer.addCallback(g_localHolder);
        g_localRenderer.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        return  localRender;
    }

    public static boolean isHolderReady(){
    	return g_localHolder.isHolderReady;
    }
    
    public static SurfaceHolder getLocalHolder(){
    	return g_localHolder.holder;
    }
    public static SurfaceHolder GetLocalRenderer() {
        return g_localRenderer;
    }

}
