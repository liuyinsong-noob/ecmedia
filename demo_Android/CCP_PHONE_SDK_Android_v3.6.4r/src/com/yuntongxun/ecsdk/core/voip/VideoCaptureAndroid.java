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

import java.util.concurrent.locks.ReentrantLock;

import com.yuntongxun.ecsdk.core.voip.CaptureCapabilityAndroid;
import com.yuntongxun.ecsdk.core.voip.VideoCaptureDeviceInfoAndroid.AndroidVideoCaptureDevice;

import com.hisun.phone.core.voice.util.Log4Util;

import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;

public class VideoCaptureAndroid implements PreviewCallback, Callback {

    private final static String TAG = "console";

    private Camera camera;
    private AndroidVideoCaptureDevice currentDevice = null;
    public ReentrantLock previewBufferLock = new ReentrantLock();
    // This lock takes sync with StartCapture and SurfaceChanged
    private ReentrantLock captureLock = new ReentrantLock();
    private int PIXEL_FORMAT = ImageFormat.NV21;
    PixelFormat pixelFormat = new PixelFormat();
    // True when the C++ layer has ordered the camera to be started.
    private boolean isCaptureStarted = false;
    private boolean isCaptureRunning = false;
    private boolean isSurfaceReady = false;
    private SurfaceHolder surfaceHolder = null;

    private final int numCaptureBuffers = 3;
    private int expectedFrameSize = 0;
    private int orientation = 0;
    private int id = 0;
    // C++ callback context variable.
    private long context = 0;
    private SurfaceHolder localPreview = null;
    // True if this class owns the preview video buffers.
    private boolean ownsBuffers = false;

    private int mCaptureWidth = -1;
    private int mCaptureHeight = -1;
    private int mCaptureFPS = -1;

    public static
    void DeleteVideoCaptureAndroid(VideoCaptureAndroid captureAndroid) {
    	if(ViESurfaceRenderer.DEBUG){
    		Log4Util.d(TAG, "DeleteVideoCaptureAndroid");
    	}

        captureAndroid.StopCapture();
        captureAndroid.camera.release();
        captureAndroid.camera = null;
        captureAndroid.context = 0;
    }

    public VideoCaptureAndroid(int in_id, long in_context, Camera in_camera,
            AndroidVideoCaptureDevice in_device) {
        id = in_id;
        context = in_context;
        camera = in_camera;
        currentDevice = in_device;
    }

    private int tryStartCapture(int width, int height, int frameRate) {
        if (camera == null) {
        	if(ViESurfaceRenderer.DEBUG){
        		Log4Util.e(TAG, "Camera not initialized %d" + id);
        	}
            return -1;
        }
        if(ViESurfaceRenderer.DEBUG){
        	
        	Log4Util.d(TAG, "tryStartCapture width: " + width +
        			",height:" + height +",frame rate:" + frameRate +
        			",isCaptureRunning:" + isCaptureRunning +
        			",isSurfaceReady:" + isSurfaceReady +
        			",isCaptureStarted:" + isCaptureStarted);
        }

        if (isCaptureRunning || !isSurfaceReady || !isCaptureStarted) {
            return 0;
        }

        try {
            camera.setPreviewDisplay(surfaceHolder);

            CaptureCapabilityAndroid currentCapability =
                    new CaptureCapabilityAndroid();
            currentCapability.width = width;
            currentCapability.height = height;
            currentCapability.maxFPS = frameRate;
            PixelFormat.getPixelFormatInfo(PIXEL_FORMAT, pixelFormat);

            Camera.Parameters parameters = camera.getParameters();
            parameters.setPreviewSize(currentCapability.width,
                    currentCapability.height);
            parameters.setPreviewFormat(PIXEL_FORMAT);
            parameters.setPreviewFrameRate(currentCapability.maxFPS);
            if(ViESurfaceRenderer.DEBUG){
            	Log4Util.i(TAG, ""+"ha");

            	Log4Util.i(TAG, "set rotation:" + currentDevice.orientation);
            }
//            parameters.setRotation(currentDevice.orientation);
//            parameters.set("orientation", "portrait");
//            parameters.set("rotation", currentDevice.orientation);
            camera.setParameters(parameters);
            
            //camera.setDisplayOrientation(currentDevice.orientation);

            int bufSize = width * height * pixelFormat.bitsPerPixel / 8;
            byte[] buffer = null;
            for (int i = 0; i < numCaptureBuffers; i++) {
                buffer = new byte[bufSize];
                camera.addCallbackBuffer(buffer);
            }
            previewBufferLock.lock();
            expectedFrameSize = bufSize;
            if(ViESurfaceRenderer.DEBUG){
            	
            	Log4Util.i(TAG, "expectedFrameSize="+bufSize);
            }
            isCaptureRunning = true;
            previewBufferLock.unlock();
            camera.setPreviewCallbackWithBuffer(this);
            ownsBuffers = true;
            camera.startPreview();

        }
        catch (Exception ex) {
        	if(ViESurfaceRenderer.DEBUG){
        		
        		Log4Util.e(TAG, "Failed to start camera");
        	}
            return -1;
        }

        isCaptureRunning = true;
        return 0;
    }

    public int StartCapture(int width, int height, int frameRate) {
    	if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "StartCapture width " + width +
    				" height " + height +" frame rate " + frameRate);
    	}
        // Get the local preview SurfaceHolder from the static render class
        localPreview = ViERenderer.GetLocalRenderer();
        if(ViESurfaceRenderer.DEBUG){
        	
        	Log4Util.d(TAG, "start capture :" + localPreview);
        }
        if (localPreview != null) {
        	if(ViERenderer.isHolderReady()){
        		surfaceHolder = ViERenderer.getLocalHolder();
        		isSurfaceReady = true;
        	}else
        		localPreview.addCallback(this);
        }

        captureLock.lock();
        isCaptureStarted = true;
        mCaptureWidth = width;
        mCaptureHeight = height;
        mCaptureFPS = frameRate;

        int res = tryStartCapture(mCaptureWidth, mCaptureHeight, mCaptureFPS);

        captureLock.unlock();
        return res;
    }

    public int StopCapture() {
    	if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "StopCapture");
    	}
        try {
            previewBufferLock.lock();
            isCaptureRunning = false;
            previewBufferLock.unlock();
            camera.stopPreview();
            camera.setPreviewCallbackWithBuffer(null);
        }
        catch (Exception ex) {
        	if(ViESurfaceRenderer.DEBUG){
        		
        		Log4Util.e(TAG, "Failed to stop camera");
        	}
            return -1;
        }

        isCaptureStarted = false;
        return 0;
    }

    native void ProvideCameraFrame(byte[] data, int length, long captureObject);

    public void onPreviewFrame(byte[] data, Camera camera) {
        previewBufferLock.lock();
        if(ViESurfaceRenderer.DEBUG){
        	
        	Log4Util.e(TAG, "onPreviewFram : len="+data.length+",expect:"+expectedFrameSize);
        }
        // The following line is for debug only
        // Log.v(TAG, "preview frame length " + data.length +
        //            " context" + context);
        if (isCaptureRunning) {
            // If StartCapture has been called but not StopCapture
            // Call the C++ layer with the captured frame
        	
            if (data.length == expectedFrameSize) {
                ProvideCameraFrame(data, expectedFrameSize, context);
                if (ownsBuffers) {
                    // Give the video buffer to the camera service again.
                    camera.addCallbackBuffer(data);
                }
            }
        }
        previewBufferLock.unlock();
    }

    // Sets the rotation of the preview render window.
    // Does not affect the captured video image.
    public void SetPreviewRotation(int rotation) {
    	if(true) { //ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.v(TAG, "SetPreviewRotation:" + rotation);
    	}

        if (camera != null) {
            previewBufferLock.lock();
            int width = 0;
            int height = 0;
            int framerate = 0;

            if (isCaptureRunning) {
                width = mCaptureWidth;
                height = mCaptureHeight;
                framerate = mCaptureFPS;
                StopCapture();
            }

            int resultRotation = 0;
            if (currentDevice.frontCameraType ==
                    VideoCaptureDeviceInfoAndroid.FrontFacingCameraType.Android23) {
                // this is a 2.3 or later front facing camera.
                // SetDisplayOrientation will flip the image horizontally
                // before doing the rotation.
                resultRotation=(360-rotation) % 360; // compensate the mirror
            }
            else {
                // Back facing or 2.2 or previous front camera
                resultRotation=rotation;
            }
            camera.setDisplayOrientation(resultRotation);

            if (isCaptureRunning) {
                StartCapture(width, height, framerate);
            }
            previewBufferLock.unlock();
        }
    }

    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
    	//if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "VideoCaptureAndroid::surfaceChanged");
    	//}

        captureLock.lock();
        isSurfaceReady = true;
        surfaceHolder = holder;

        tryStartCapture(mCaptureWidth, mCaptureHeight, mCaptureFPS);
        captureLock.unlock();
        return;
    }

    public void surfaceCreated(SurfaceHolder holder) {
    	//if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "VideoCaptureAndroid::surfaceCreated");
    	//}
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    	//if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "VideoCaptureAndroid::surfaceDestroyed");
    	//}
        isSurfaceReady = false;
    }
}
