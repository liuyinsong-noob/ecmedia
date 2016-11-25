package com.yuntongxun.ecsdk.core.jni;

/**
 * Created by Administrator on 8/9 0009.
 */
public class OpenGLESDisplay {

    public static void init() {
        System.loadLibrary("opengles");
    }
    /*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    Create
 * Signature: ()I
 */
    public static native int Create();

/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    Remove
 * Signature: (I)V
 */
    public static native void Remove(int id);

/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    Init
 * Signature: (III)V
 */
    public static native void Init(int id, int w, int h);

/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    Uninit
 * Signature: (I)V
 */
    public static native void Uninit(int id);
/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    SetZoom
 * Signature: (IFFF)V
 */
    public static native void SetZoom(int id, float factor, float cx, float cy);
/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    SetYuv
 * Signature: (III[BI)V
 */
    public static native void SetYuv(int id, int w, int h, byte[] yuv, int len);
/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    SetPreviewYuv
 * Signature: (III[BI)V
 */
    public static native void SetPreviewYuv(int id, int w, int h, byte[] yuv, int len);
/*
 * Class:     com_yuntongxun_ecsdk_core_jni_OpenGLESDisplay
 * Method:    Render
 * Signature: (I)V
 */
    public static native void Render(int id);
}
