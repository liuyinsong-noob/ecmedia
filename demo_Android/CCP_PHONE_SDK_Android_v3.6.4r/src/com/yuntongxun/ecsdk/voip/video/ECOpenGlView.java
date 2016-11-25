/*
 *  Copyright (c) 2015 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */package com.yuntongxun.ecsdk.voip.video;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.widget.RelativeLayout;

import com.hisun.phone.core.voice.util.Log4Util;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

/**
 * <p>标准的使用代码如下:</p>
 * <pre class="prettyprint">
 * 	 	// 初始化一个图像绘制控件
 *  	ECOpenGlView mGlView = new ECOpenGlView(getContext());
 *  	// 远端图像显示(默认)
 *  	mGlView.setGlType(ECOpenGlView.RenderType.RENDER_REMOTE);
 *  	// 或者本地预览图像
 *  	mGlView.setGlType(ECOpenGlView.RenderType.RENDER_PREVIEW);
 *  	// 图像等比按照中心区域显示屏截取
 *  	mGlView.setAspectMode(ECOpenGlView.AspectMode.CROP);
 *  	// 按照图像的分辨率完全填充显示控件，有拉伸效果
 *  	mGlView.setAspectMode(ECOpenGlView.AspectMode.FILL);
 *  	// 按照图像的宽高等比绘制，会出现有边框效果
 *  	mGlView.setAspectMode(ECOpenGlView.AspectMode.FIT);
 * </pre>
 * @author 容联•云通讯
 * @version 5.2.2
 * @since 2015-10-25
 */
public class ECOpenGlView extends GLSurfaceView {

    private static final String TAG = "OPENGLVIEW";//ECLogger.getLogger(ECOpenGlView.class);

    /**
     * 当前Gl绘制模式
     */
    public enum AspectMode {
        /**按照图像的宽高等比绘制，会出现有边框效果*/
        FIT,
        /**按照控件的大小对图像进行截取，并显示图像中间内容填充控件*/
        CROP ,
        /**按照图像的分辨率完全填充显示控件，有拉伸效果*/
        FILL ,
    }

    /**
     * 当前所创建的Gl使用类型
     */
    public enum RenderType {
        /**表示当前为本地预览数据的Render*/
        RENDER_PREVIEW ,
        /**表示当前为远端数据的Render*/
        RENDER_REMOTE ,
    }

    /**当前所使用的Open GL的版本号*/
    private int mGlVersion = 1;
    /**视频数据Render*/
    private WeakReference<Renderer> mRenders;
    /**本地预览数据绘制对象*/
    long mPtr = -1;
    private String mRenderUser;
    /**当前的绘制模式*/
    AspectMode mAspectMode;
    /**当前绘制对象所用与的场景*/
    protected RenderType mRenderType;

    public ECOpenGlView(Context context) {
        this(context , null);
    }

    public ECOpenGlView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mGlVersion = ECOpenGlRender.getOpenGLVersion(context);
        getHolder().addCallback(this);
        initOpenGlView();
        mRenderType = RenderType.RENDER_REMOTE;
        initRenderer(new ECOpenGlRender(this , mRenderType.ordinal()));
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    /**
     * 初始化OpenGL绘制界面
     */
    @SuppressWarnings("deprecation")
    private void initOpenGlView() {
        try {
            getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
        } catch (Exception e) {
            if (!setSurfaceType(SurfaceHolder.SURFACE_TYPE_HARDWARE)) {
                setSurfaceType(SurfaceHolder.SURFACE_TYPE_NORMAL);
            }
        }

        if(mGlVersion == 2) {
            // Setup the context factory for 2.0 rendering.
            // See ContextFactory class definition below
            setEGLContextFactory(new ContextFactory());
            // We need to choose an EGLConfig that matches the format of
            // our surface exactly. This is going to be done in our
            // custom config chooser. See ConfigChooser class definition
            // below.
            setEGLConfigChooser(new ConfigChooser());
        }
    }

    @SuppressWarnings("deprecation")
    private boolean setSurfaceType(int type) {
        try {
            getHolder().setType(type);
            return true;
        } catch (Exception e) {
            Log4Util.e(TAG, e + "get Exception on setSurfaceType");

        }
        return false;
    }

    /**
     * 设置当前Gl对象的绘制类型
     * @param type 绘制类型
     */
    public void setGlType(RenderType type) {
        mRenderType = type;
        initZOrderAttribute(type == RenderType.RENDER_PREVIEW);
    }

    /**
     * 初始化OpenGl绘制属性
     */
    private void initZOrderAttribute(boolean isPreview) {
        ECOpenGlRender mRender = (ECOpenGlRender) mRenders.get();
        if(mRender == null || mRender.mSurfaceReady) {
            Log4Util.e(TAG , "setGlType fail, mRender null or surfaceReady ");
            return ;
        }
        if(!isPreview) {
            return;
        }
        if (Build.MODEL.equals("Nexus 6")) {
            setZOrderOnTop(true);
        } else {
            setZOrderMediaOverlay(true);
        }
    }

    /**
     * 设置当前控件宽和高
     * @param width 宽
     * @param height 高
     */
    public void setGlViewSize(int width , int height) {
        setLayoutParams(new RelativeLayout.LayoutParams(width, height));
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    /**
     * 设置当前绘制界面所属的用户帐号
     * @param account 账号
     */
    public void setGlUser(String account) {
//        if(ECSDKUtils.isNullOrNil(account)) {
//            throw new IllegalArgumentException("ECOpenGlView must set mark , can't null");
//        }
        mRenderUser = account;
    }

    /**
     * 设置当前绘制界面所绑定的用户帐号
     * @return 所绑定的用户帐号
     */
    public String getGlUser() {
        return mRenderUser;
    }

    /**
     * 设置视频图像Render对象
     * @param renderer 视频图像Render对象
     */
    private void initRenderer(ECOpenGlRender renderer) {
        if(mRenders != null) {
            Log4Util.e(TAG , "glRender has init.");
            return ;
        }
        setEGLContextClientVersion(mGlVersion);
        mRenders = new WeakReference<Renderer>(renderer);
        Log4Util.i(TAG , "initRenderer "+renderer+" , glVersion "+ mGlVersion);
        super.setRenderer(renderer);
    }

    /**
     * 设置当前图像的显示模式，如果不设置则按照默认值缩放
     * @param mode 显示模式
     *
     */
    public void setAspectMode(AspectMode mode) {
        mAspectMode = mode;
    }

    /**
     * 返回当前绘制视频图像Render对象
     * @return 视频图像Render对象
     */
    public ECOpenGlRender getRenderer() {
        if(mRenders == null) {
            return null;
        }
        return (ECOpenGlRender) mRenders.get();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log4Util.i(TAG , "surfaceChanged, format: "+format+", w: "+w+", h: "+ h);
        super.surfaceChanged(holder, format, w, h);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        super.surfaceCreated(holder);
        Log4Util.i(TAG , "surfaceCreated");
        ECOpenGlRender mRender = (ECOpenGlRender) mRenders.get();
        if(mRender == null) {
            return ;
        }
        mPtr = mRender.Create();
        if(mPtr == -1) {
            return ;
        }
        Log4Util.i(TAG , "surfaceCreated glRender init");
        /*mRender.Init(mPtr , mRender.mGlViews.get().getWidth() , mRender.mGlViews.get().getHeight());
        mRender.setParam(mPtr , mRender.mZoomFactor, mRender.mZoomCx, mRender.mZoomCy);
        mRender.mGlInitReady = true;*/
        mRender.mSurfaceReady = true;
        mRender.mWidth = 0;
        mRender.mHeight = 0;
        mRender.requestRender();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log4Util.i(TAG , "surfaceDestroyed");
        ECOpenGlRender mRender = (ECOpenGlRender) mRenders.get();
        if(mRender != null) {
            mRender.mSurfaceReady = false;
            if(mPtr != -1) {
                mRender.Uninit(mPtr);
            }
            mRender.mWidth = 0;
            mRender.mHeight = 0;
            mRender.mGlInitReady = false;
        }
        super.surfaceDestroyed(holder);
    }

    private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            Log4Util.w(TAG , "creating OpenGL ES 2.0 context");
            Log4Util.v(TAG , "Before eglCreateContext "+egl);
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
            EGLContext context = egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
            Log4Util.v(TAG, "After eglCreateContext "+egl);
            return context;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            egl.eglDestroyContext(display, context);
        }
    }


    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

        // This EGL config specification is used to specify 2.0 rendering.
        // We use a minimum size of 4 bits for red/green/blue, but will
        // perform actual matching in chooseConfig() below.
        private static int EGL_OPENGL_ES2_BIT = EGL10.EGL_WINDOW_BIT;
        private static int[] s_configAttribs2 = {
                EGL10.EGL_RED_SIZE,
                EGL_OPENGL_ES2_BIT,
                EGL10.EGL_GREEN_SIZE,
                EGL_OPENGL_ES2_BIT,
                EGL10.EGL_BLUE_SIZE,
                EGL_OPENGL_ES2_BIT,
                EGL10.EGL_RENDERABLE_TYPE,
                EGL_OPENGL_ES2_BIT,
                EGL10.EGL_NONE
        };

        // Subclasses can adjust these values:
        int mRedSize = 5;
        int mGreenSize = 6;
        int mBlueSize = 5;
        int mAlphaSize = 0;
        int mDepthSize = 0;
        int mStencilSize = 0;
        private int[] mValue = new int[1];

        @Override
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

            // Now return the "best" one
            return chooseConfig(egl, display, configs);
        }

        EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                               EGLConfig[] configs) {
            for(EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE , 0);
                int s = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);

                // We need at least mDepthSize and mStencilSize bits
                if (d < mDepthSize || s < mStencilSize)
                    continue;

                // We want an *exact* match for red/green/blue/alpha
                int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);

                if (r == mRedSize && g == mGreenSize && b == mBlueSize && a == mAlphaSize) {
                    return config;
                }
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display,
                                     EGLConfig config, int attribute , int defaultAttrib) {

            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultAttrib;
        }
    }

    public static boolean UseOpenGL2(Object renderWindow) {
        return renderWindow != null && ECOpenGlView.class.isInstance(renderWindow);
    }

}
