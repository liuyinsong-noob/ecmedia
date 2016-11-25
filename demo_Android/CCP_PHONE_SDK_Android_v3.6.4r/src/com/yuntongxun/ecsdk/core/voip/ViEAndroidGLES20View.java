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
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

import com.hisun.phone.core.voice.util.Log4Util;
import com.yuntongxun.ecsdk.core.jni.OpenGLESDisplay;

import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

public class ViEAndroidGLES20View extends GLSurfaceView
        implements GLSurfaceView.Renderer {
    private static String TAG = "WEBRTC-JR";
    private static final boolean DEBUG = true;
    // True if onSurfaceCreated has been called.
    private boolean surfaceCreated = false;
    private boolean openGLCreated = false;
    private ReentrantLock nativeFunctionLock = new ReentrantLock();
    // Address of Native object that will do the drawing.
    private long nativeObject = 0;
    private int viewWidth = 0;
    private int viewHeight = 0;

    private int opengles = 0;
    private int openglesID = 0;

    public ViEAndroidGLES20View(Context context, AttributeSet attr)
    {
        super(context, attr);
        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View 0");
        init(false, 0, 0);
    }

    public ViEAndroidGLES20View(Context context) {
        super(context);
        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View 1");
        init(false, 0, 0);
    }

    public ViEAndroidGLES20View(Context context, boolean translucent,
                                int depth, int stencil) {
        super(context);
        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View 2");
        init(translucent, depth, stencil);
    }

    private void init(boolean translucent, int depth, int stencil) {

        // By default, GLSurfaceView() creates a RGB_565 opaque surface.
        // If we want a translucent one, we should change the surface's
        // format here, using PixelFormat.TRANSLUCENT for GL Surfaces
        // is interpreted as any 32-bit surface with alpha by SurfaceFlinger.
//        if (translucent) {
//            this.getHolder().setFormat(PixelFormat.TRANSLUCENT);
//        }
        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View init 0");
        // Setup the context factory for 2.0 rendering.
        // See ContextFactory class definition below
        setEGLContextFactory(new ContextFactory());
        //setEGLContextClientVersion(2);

        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View init 1");

        // We need to choose an EGLConfig that matches the format of
        // our surface exactly. This is going to be done in our
        // custom config chooser. See ConfigChooser class definition
        // below.
        setEGLConfigChooser( translucent ?
                             new ConfigChooser(8, 8, 8, 8, depth, stencil) :
                             new ConfigChooser(5, 6, 5, 0, depth, stencil) );

        // Set the renderer responsible for frame rendering
        this.setRenderer(this);
        this.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View init 2");

        OpenGLESDisplay.init();

        if(DEBUG)
            Log4Util.d("ViEAndroidGLES20View init 3");
    }

    protected void finalize () {
        surfaceCreated = false;
        OpenGLESDisplay.Uninit(openglesID);
        OpenGLESDisplay.Remove(openglesID);
        //super.finalize();
    }

    private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
        	if(DEBUG)
        		Log4Util.w(TAG, "ViEAndroidGLES20View creating OpenGL ES 2.0 context 0");

            checkEglError("Before eglCreateContext", egl);
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE};
            EGLContext context = egl.eglCreateContext(display, eglConfig,
                    EGL10.EGL_NO_CONTEXT, attrib_list);
            checkEglError("After eglCreateContext", egl);

            if(DEBUG)
                Log4Util.w(TAG, "ViEAndroidGLES20View creating OpenGL ES 2.0 context 1");
            return context;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            egl.eglDestroyContext(display, context);
        }
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
        	if(ViESurfaceRenderer.DEBUG){
        		Log4Util.e(TAG, String.format("ViEAndroidGLES20View %s: EGL error: 0x%x", prompt, error));
        	}
        }
    }

    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

        public ConfigChooser(int r, int g, int b, int a, int depth, int stencil) {
            mRedSize = r;
            mGreenSize = g;
            mBlueSize = b;
            mAlphaSize = a;
            mDepthSize = depth;
            mStencilSize = stencil;
        }

        // This EGL config specification is used to specify 2.0 rendering.
        // We use a minimum size of 4 bits for red/green/blue, but will
        // perform actual matching in chooseConfig() below.
        private static int EGL_OPENGL_ES2_BIT = 4;
        private static int[] s_configAttribs2 =
        {
            EGL10.EGL_RED_SIZE, 4,
            EGL10.EGL_GREEN_SIZE, 4,
            EGL10.EGL_BLUE_SIZE, 4,
            EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL10.EGL_NONE
        };

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {

            // Get the number of minimally matching EGL configurations
            int[] num_config = new int[1];
            egl.eglChooseConfig(display, s_configAttribs2, null, 0, num_config);

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                throw new IllegalArgumentException("No configs match configSpec");
            }

            // Allocate then read the array of minimally matching EGL configs
            EGLConfig[] configs = new EGLConfig[numConfigs];
            egl.eglChooseConfig(display, s_configAttribs2, configs, numConfigs, num_config);

            if (DEBUG) {
                printConfigs(egl, display, configs);
            }
            // Now return the "best" one
            return chooseConfig(egl, display, configs);
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                EGLConfig[] configs) {
            for(EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config,
                        EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib(egl, display, config,
                        EGL10.EGL_STENCIL_SIZE, 0);

                // We need at least mDepthSize and mStencilSize bits
                if (d < mDepthSize || s < mStencilSize)
                    continue;

                // We want an *exact* match for red/green/blue/alpha
                int r = findConfigAttrib(egl, display, config,
                        EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib(egl, display, config,
                            EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib(egl, display, config,
                            EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib(egl, display, config,
                        EGL10.EGL_ALPHA_SIZE, 0);

                if (r == mRedSize && g == mGreenSize && b == mBlueSize && a == mAlphaSize)
                    return config;
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display,
                EGLConfig config, int attribute, int defaultValue) {

            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }

        private void printConfigs(EGL10 egl, EGLDisplay display,
            EGLConfig[] configs) {
            int numConfigs = configs.length;
            if(ViESurfaceRenderer.DEBUG){
        		Log4Util.w(TAG, String.format("%d configurations", numConfigs));
        	}
            for (int i = 0; i < numConfigs; i++) {
            	if(ViESurfaceRenderer.DEBUG){
            		Log4Util.w(TAG, String.format("Configuration %d:\n", i));
            	}
                printConfig(egl, display, configs[i]);
            }
        }

        private void printConfig(EGL10 egl, EGLDisplay display,
                EGLConfig config) {
            int[] attributes = {
                    EGL10.EGL_BUFFER_SIZE,
                    EGL10.EGL_ALPHA_SIZE,
                    EGL10.EGL_BLUE_SIZE,
                    EGL10.EGL_GREEN_SIZE,
                    EGL10.EGL_RED_SIZE,
                    EGL10.EGL_DEPTH_SIZE,
                    EGL10.EGL_STENCIL_SIZE,
                    EGL10.EGL_CONFIG_CAVEAT,
                    EGL10.EGL_CONFIG_ID,
                    EGL10.EGL_LEVEL,
                    EGL10.EGL_MAX_PBUFFER_HEIGHT,
                    EGL10.EGL_MAX_PBUFFER_PIXELS,
                    EGL10.EGL_MAX_PBUFFER_WIDTH,
                    EGL10.EGL_NATIVE_RENDERABLE,
                    EGL10.EGL_NATIVE_VISUAL_ID,
                    EGL10.EGL_NATIVE_VISUAL_TYPE,
                    0x3030, // EGL10.EGL_PRESERVED_RESOURCES,
                    EGL10.EGL_SAMPLES,
                    EGL10.EGL_SAMPLE_BUFFERS,
                    EGL10.EGL_SURFACE_TYPE,
                    EGL10.EGL_TRANSPARENT_TYPE,
                    EGL10.EGL_TRANSPARENT_RED_VALUE,
                    EGL10.EGL_TRANSPARENT_GREEN_VALUE,
                    EGL10.EGL_TRANSPARENT_BLUE_VALUE,
                    0x3039, // EGL10.EGL_BIND_TO_TEXTURE_RGB,
                    0x303A, // EGL10.EGL_BIND_TO_TEXTURE_RGBA,
                    0x303B, // EGL10.EGL_MIN_SWAP_INTERVAL,
                    0x303C, // EGL10.EGL_MAX_SWAP_INTERVAL,
                    EGL10.EGL_LUMINANCE_SIZE,
                    EGL10.EGL_ALPHA_MASK_SIZE,
                    EGL10.EGL_COLOR_BUFFER_TYPE,
                    EGL10.EGL_RENDERABLE_TYPE,
                    0x3042 // EGL10.EGL_CONFORMANT
            };
            String[] names = {
                    "EGL_BUFFER_SIZE",
                    "EGL_ALPHA_SIZE",
                    "EGL_BLUE_SIZE",
                    "EGL_GREEN_SIZE",
                    "EGL_RED_SIZE",
                    "EGL_DEPTH_SIZE",
                    "EGL_STENCIL_SIZE",
                    "EGL_CONFIG_CAVEAT",
                    "EGL_CONFIG_ID",
                    "EGL_LEVEL",
                    "EGL_MAX_PBUFFER_HEIGHT",
                    "EGL_MAX_PBUFFER_PIXELS",
                    "EGL_MAX_PBUFFER_WIDTH",
                    "EGL_NATIVE_RENDERABLE",
                    "EGL_NATIVE_VISUAL_ID",
                    "EGL_NATIVE_VISUAL_TYPE",
                    "EGL_PRESERVED_RESOURCES",
                    "EGL_SAMPLES",
                    "EGL_SAMPLE_BUFFERS",
                    "EGL_SURFACE_TYPE",
                    "EGL_TRANSPARENT_TYPE",
                    "EGL_TRANSPARENT_RED_VALUE",
                    "EGL_TRANSPARENT_GREEN_VALUE",
                    "EGL_TRANSPARENT_BLUE_VALUE",
                    "EGL_BIND_TO_TEXTURE_RGB",
                    "EGL_BIND_TO_TEXTURE_RGBA",
                    "EGL_MIN_SWAP_INTERVAL",
                    "EGL_MAX_SWAP_INTERVAL",
                    "EGL_LUMINANCE_SIZE",
                    "EGL_ALPHA_MASK_SIZE",
                    "EGL_COLOR_BUFFER_TYPE",
                    "EGL_RENDERABLE_TYPE",
                    "EGL_CONFORMANT"
            };
            int[] value = new int[1];
            for (int i = 0; i < attributes.length; i++) {
                int attribute = attributes[i];
                String name = names[i];
                if (egl.eglGetConfigAttrib(display, config, attribute, value)) {
                	if(ViESurfaceRenderer.DEBUG){
                		Log4Util.w(TAG, String.format("  %s: %d\n", name, value[0]));
                	}
                } else {
                    // Log.w(TAG, String.format("  %s: failed\n", name));
                    while (egl.eglGetError() != EGL10.EGL_SUCCESS);
                }
            }
        }

        // Subclasses can adjust these values:
        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;
        private int[] mValue = new int[1];
    }

    // IsSupported
    // Return true if this device support Open GL ES 2.0 rendering.
    public static boolean IsSupported(Context context) {
        return false;
    }

    public void onDrawFrame(GL10 gl) {
        nativeFunctionLock.lock();

        Log4Util.e("ViEAndroidGLES20View onDrawFrame");
        if(openGLCreated && surfaceCreated) {
            OpenGLESDisplay.Render(openglesID);
        }

        nativeFunctionLock.unlock();
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {

        nativeFunctionLock.lock();
        Log4Util.e("ViEAndroidGLES20View onSurfaceChanged");

        if(!openGLCreated) {
            openglesID = OpenGLESDisplay.Create();
            openGLCreated = true; // Created OpenGL successfully
        }

        if(width != viewWidth || height != viewHeight) {
            Log4Util.d(TAG, "ViEAndroidGLES20View onSurfaceChanged w:"+width+" h:"+height);
            OpenGLESDisplay.Uninit(openglesID);
            OpenGLESDisplay.Init(openglesID, width, width);
            OpenGLESDisplay.SetZoom(openglesID,1,0,0);
            viewWidth = width;
            viewHeight = height;
            surfaceCreated = true;
        }
        nativeFunctionLock.unlock();
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log4Util.e("ViEAndroidGLES20View onSurfaceCreated");
    }

    public void ReDraw(int w, int h, byte[] data, int dataLen) {
        if(surfaceCreated) {
            Log4Util.e("ViEAndroidGLES20View ReDraw");

            OpenGLESDisplay.SetYuv(openglesID, w, h, data, dataLen);
            // Request the renderer to redraw using the render thread context.
            this.requestRender();
        }
    }
}
