package com.yuntongxun.ecsdk.voip.video;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Looper;
import android.os.Message;

import com.hisun.phone.core.voice.util.Log4Util;
import com.yuntongxun.ecsdk.voip.video.IRender;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * @author 容联•云通讯
 * @version 5.2.0
 * @since 2016-07-11
 */
public class ECOpenGlRender extends IRender implements GLSurfaceView.Renderer {

    public static final String TAG = "GLRENDER";//ECLogger.getLogger(ECOpenGlRender.class);

    /**当前手机系统可用的Open GL 版本号*/
    private static int mGlVersion = 0;
    /**Render所绑定的OpenGLView界面*/
    private WeakReference<ECOpenGlView> mGlViews;
    /**本地预览接口是否初始化完成*/
    protected boolean mGlInitReady = false;
    /**当前的预览界面是否可用*/
    boolean mSurfaceReady = false;
    private float mZoomFactor = 1.2F;
    private float mZoomCx = 1.93F;
    private float mZoomCy = 1.05F;
    /**当前可绘制区域的宽度*/
    int mWidth = 0;
    /**当前可绘制区域的高度*/
    int mHeight = 0;
    /**视频数据属性-宽*/
    private int mFrameWidth;
    /**视频数据属性-高*/
    private int mFrameHeight;
    /**视频数据属性-字节长度*/
    private int mFrameLength;
    /**视频数据-远端视频数据*/
    private byte[] mByte = null;
    /**视频数据-本地摄像头预览数据*/
    private byte[] mPreviewByte = null;
    private int mRotation = 0;
    /**当前的Render类型*/
    private int mRenderType;
    /**绘制请求发起队列*/
    //RenderHandler mRenderHandler;

    /**
     * 根据提供的Gl画板创建一个本地绘制接口
     * @param mGlView 视频数据显示画板
     * @param renderType 预览类型，标识是否本地预览或者远端数据
     */
    ECOpenGlRender(ECOpenGlView mGlView, int renderType) {
        mFrameWidth = 0;
        mFrameHeight = 0;
        mFrameLength = 0;
        mPreviewByte = null;
        mByte = null;
        mGlViews = new WeakReference<ECOpenGlView>(mGlView);
        mRenderType = renderType;
//        Looper looper = Looper.myLooper();
//        if(looper != null) {
//            mRenderHandler = new RenderHandler(looper);
//        } else {
//            looper = Looper.getMainLooper();
//            if(looper != null) {
//                mRenderHandler = new RenderHandler(looper);
//            } else {
//                mRenderHandler = null;
//            }
//        }
    }

    /**
     * 检查OpenGL版本信息
     * @return 是否支持OpenGL 2.0
     */
    static int getOpenGLVersion(Context context) {
        if(mGlVersion == 0) {
            ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            ConfigurationInfo info = am.getDeviceConfigurationInfo();
            if(info.reqGlEsVersion >= 0x20000) {
                mGlVersion = 2;
            } else {
                mGlVersion = 1;
            }
            if(Build.MODEL.equals("Nexus 6")) {
                mGlVersion = 2;
            }
        }
        Log4Util.d(TAG , "getOpenGLVersion " + mGlVersion);
        return mGlVersion;
    }

    /**
     * 绘制视频预览数据
     * @param data 视频预览数据
     * @param width 视频数据-宽
     * @param height 视频预览数据-高
     * @param length 视频预览数据-大小
     */
    public final void renderCapture(byte[] data, int width, int height, int length , int rotation) {
        if ((this.mSurfaceReady) && (this.mPreviewByte == null)) {
            this.mFrameWidth = width;
            this.mFrameHeight = height;
            this.mFrameLength = length;
            this.mPreviewByte = data;
            this.mRotation = rotation;
            requestRender();
        }
    }

    /**
     * 绘制远端视频数据
     * @param data 远端视频数据
     * @param width 视频数据-宽
     * @param height 视频预览数据-高
     * @param length 视频预览数据-大小
     */
    public final void renderFrame(byte[] data, int width, int height, int length) {
        if ((this.mSurfaceReady) && (this.mByte == null)) {
            if(this.mByte == null || this.mFrameLength != length){
                this.mByte = new byte[length];
            }

            this.mFrameWidth = width;
            this.mFrameHeight = height;
            this.mFrameLength = length;
            System.arraycopy(data, 0, this.mByte, 0, length);
            //this.mByte = data;
            requestRender();
        }
    }


    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log4Util.i(TAG , "onSurfaceChanged, width: " + width + ", height: " + height);
        if(mWidth != width || mHeight != height) {
            gl.glViewport(0 , 0 , width , height);
            mWidth = width;
            mHeight = height;
        }
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        ECOpenGlView mGlView = mGlViews.get();
        if(!mSurfaceReady || mGlView == null) {
            mByte = null;
            mPreviewByte = null;
            return ;
        }
        if(!mGlInitReady) {
            if(mGlView.mPtr == -1) {
                mGlView.mPtr = Create();
            }
            Init(mGlView.mPtr , mGlView.getWidth() , mGlView.getHeight());
            int mAspect = (mGlView.mAspectMode == null) ? -1 : mGlView.mAspectMode.ordinal();
            setParam(mGlView.mPtr , mZoomFactor, mZoomCx, mZoomCy , mAspect);
            mGlInitReady = true;
        }
        if(mPreviewByte != null) {
            SetPreviewYuv(mGlView.mPtr , mFrameWidth , mFrameHeight , mPreviewByte ,mFrameLength , mRotation);
            mPreviewByte = null;
            return ;
        }
        if(mByte != null) {
            SetYuv(mGlView.mPtr, mFrameWidth, mFrameHeight, mByte , mFrameLength);
            mByte = null;
        }
    }

    /**
     * 通知本地数据准备完毕，开始绘制数据
     */
    void requestRender() {
        Log4Util.i(TAG , "requestRender mSurfaceReady  " + mSurfaceReady);
        if(!mSurfaceReady) {
            return ;
        }
        ECOpenGlView glView = mGlViews.get();
        if(glView == null) {
            Log4Util.e(TAG , "requestRender fail glView null");
            return ;
        }
        glView.requestRender();
        Log4Util.v(TAG , "requestRender success " + mRotation);
    }

    public native long Create();

    public native void Remove(long ptr);

    public native void Init(long ptr, int width, int height);

    public native void Uninit(long ptr);

    public native void setParam(long ptr, float factor, float cx, float cy , int AspectMode);

    public native void SetYuv(long ptr, int width, int height, byte[] yuv, int len);

    public native void SetPreviewYuv(long ptr, int width, int height, byte[] yuv, int len , int rotation);

    /**
     * 绘制动作发起队列
     */
//    private final class RenderHandler extends CCPHandler {
//
//        RenderHandler(Looper looper) {
//            super(looper);
//        }
//
//        @Override
//        public void handleMessage(Message msg) {
//            super.handleMessage(msg);
//            requestRender();
//        }
//    }

    static {
        System.loadLibrary("yuntx_gl_disp");
    }


}
