/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license 
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "screen_capturer.h"

#include "desktop_frame.h"

#include <assert.h>

#include "../system_wrappers/include/scoped_ptr.h"
#include "../system_wrappers/include/trace.h"

#include <jni.h>
#include <Android/bitmap.h>

#define AndroidJavaScreenCaptureClass "com/yuntongxun/ecsdk/core/voip/ScreenCaptureAndroid"

namespace yuntongxunwebrtc {

class ScreenCapturerAndroid : public ScreenCapturer {

public:
  ScreenCapturerAndroid();
  virtual ~ScreenCapturerAndroid();

  // WindowCapturer interface.
  bool GetShareCaptureRect(int &width, int &height);

  // DesktopCapturer interface.
  void Start(Callback* callback) ;
  void Capture(const DesktopRegion& region) ;

  virtual bool GetScreenList(ScreenList* screens);
    
  // Select the screen to be captured. Returns false in case of a failure (e.g.
  // if there is no screen with the specified id). If this is never called, the
  // full desktop is captured.
  virtual bool SelectScreen(ScreenId id);
  
  virtual void SetScreenShareActivity(void * activity);

  static void JNICALL ProvideScreeData(JNIEnv * env,
                                         jobject,
                                         jobject obj,
                                         jbyteArray javaDesktopFrame,
                                         jint data_len,
                                         jint width,
                                         jint height,
                                         jlong context);
    
public:
  static ScreenCapturerAndroid* current_obj_prt;
  static JavaVM* g_jvm;
  static jobject g_javaContext; // Java Application context  
  //ScreenCaptureAndroid.java
  static jclass g_javaScreenCaptureClass;
  //Static java object implementing the needed device info functions;
  static jobject g_javaScreenCaptureObject;
  Callback* callback_;
  
private:
  bool IsAeroEnabled();
  DesktopSize previous_size_;
  void * activity_;  
  DISALLOW_COPY_AND_ASSIGN(ScreenCapturerAndroid);
};

ScreenCapturerAndroid* ScreenCapturerAndroid::current_obj_prt = NULL;
JavaVM* ScreenCapturerAndroid::g_jvm = NULL;
jobject ScreenCapturerAndroid::g_javaContext = NULL;
//ScreenCaptureAndroid.java
jclass ScreenCapturerAndroid::g_javaScreenCaptureClass = NULL;
//static instance of ScreenCaptureAndroid.java
jobject ScreenCapturerAndroid::g_javaScreenCaptureObject = NULL;

WebRtc_Word32 ScreenCapturer::SetAndroidObjects(void* javaVM, void* env, void* javaContext)
{
  ScreenCapturerAndroid::g_jvm = static_cast<JavaVM*> (javaVM);
	ScreenCapturerAndroid::g_javaContext = static_cast<jobject> (javaContext);
	JNIEnv* thisEnv = (JNIEnv*)env;
  
  WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, -1,
	            "[ScreenCapturerAndroid INFO]: set android objects: jvm:%p, context:%p, env:%p", ScreenCapturerAndroid::g_jvm, ScreenCapturerAndroid::g_javaContext, thisEnv);
	// get java capture class type (note path to class packet)	               
	jclass screenCaptureClassLocal = thisEnv->FindClass(AndroidJavaScreenCaptureClass);
	if (!screenCaptureClassLocal) {
	  WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, -1,
	            "%s: could not find java class: %s", __FUNCTION__, AndroidJavaScreenCaptureClass);
	  return -1;
	}

	// create a global reference to the class
	// (to tell JNI that we are referencing it
	// after this function has returned)
	ScreenCapturerAndroid::g_javaScreenCaptureClass = static_cast<jclass>(thisEnv->NewGlobalRef(screenCaptureClassLocal));

  if (!ScreenCapturerAndroid::g_javaScreenCaptureClass) {
	  WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, -1,
 	               "%s: could not create android screen capture class reference",
	                __FUNCTION__);
	  return -1;
	}
	// Delete local class ref, we only use the global ref
	thisEnv->DeleteLocalRef(screenCaptureClassLocal);
	
  
    JNINativeMethod nativeFunctions = { 
      "ProvideScreeData", "(Ljava/lang/Object;[BIII)V",
        (void*) &ScreenCapturerAndroid::ProvideScreeData 
        };
	if (thisEnv->RegisterNatives(ScreenCapturerAndroid::g_javaScreenCaptureClass, &nativeFunctions, 1) == 0) {
	    WEBRTC_TRACE(yuntongxunwebrtc::kTraceDebug, yuntongxunwebrtc::kTraceVideoCapture, -1, "%s: Registered native functions", __FUNCTION__);
	}	else {
	  WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, -1,
	              "%s: Failed to register native functions",
	              __FUNCTION__);
	  return -1;
	}

  // get the method ID for the void(void) constructor
  jmethodID cid = thisEnv->GetMethodID(ScreenCapturerAndroid::g_javaScreenCaptureClass, "<init>", "()V");
  if (cid == NULL) {
       	 WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, -1,
	              "%s: Failed to get init method.",
	              __FUNCTION__);
    return -1;
  }

  // construct the object
  jobject javaObjLocal = thisEnv->NewObject(ScreenCapturerAndroid::g_javaScreenCaptureClass, cid);
  if (!javaObjLocal)
  {
    WEBRTC_TRACE(kTraceWarning, kTraceVideoCapture, 0,
                "%s: could not create Java sc object", __FUNCTION__);
    return -1;
  }

  ScreenCapturerAndroid::g_javaScreenCaptureObject = thisEnv->NewGlobalRef(javaObjLocal);
  if (!ScreenCapturerAndroid::g_javaScreenCaptureObject)
  {
    WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
                "%s: could not create Java sc object reference",
                __FUNCTION__);
    return -1;
  }
	// Delete local object ref, we only use the global ref
	thisEnv->DeleteLocalRef(javaObjLocal);
	
}
  
ScreenCapturerAndroid::ScreenCapturerAndroid()
    : callback_(NULL){
    activity_ = NULL;
    // get current object ptr;
    ScreenCapturerAndroid::current_obj_prt = this;
}

ScreenCapturerAndroid::~ScreenCapturerAndroid() {

}

bool ScreenCapturerAndroid::IsAeroEnabled() {
  bool result = false;
  return result;
}

bool  ScreenCapturerAndroid::GetShareCaptureRect(int &width, int &height)
{    
  if (!activity_) {
      WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
                  "%s activity_ is null", __FUNCTION__);
      return false;
  }

  // get the JNI env for this thread
  JNIEnv *env;
  bool isAttached = false;

  // get the JNI env for this thread
  if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
  {
    // try to attach the thread and get the env
    // Attach this thread to JVM
    jint res = g_jvm->AttachCurrentThread(&env, NULL);
    if ((res < 0) || !env)
    {
      WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,  "Could not attach thread to JVM (%d, %p)", res, env);
      return -1;
    }
    isAttached = true;
  }

  // get the method ID
  jmethodID screenRectMethodID = env->GetMethodID(g_javaScreenCaptureClass, "GetScreenRect", "()[I");
  if(screenRectMethodID == NULL) {
    WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0, "[ScreenCapturerAndroid]: can not find method: screenRectMethodID");
    return -1;
  }
  
  jintArray screenRect = (jintArray)env->CallObjectMethod( g_javaScreenCaptureObject, screenRectMethodID);
  WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0, "[ScreenCapturerAndroid]: finash call method screenRectMethodID.");
    
  jint *rect = env->GetIntArrayElements(screenRect, false);  
  if(rect == NULL) {  
    WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0, "[ScreenCapturerAndroid]: get screen rect array element faild.");
    return -1; /* exception occurred */  
  }  
  width = rect[0];
  height = rect[1];
  env->ReleaseIntArrayElements(screenRect, rect, 0);   

  // Detach this thread if it was attached
  if (isAttached)
  {
    if (g_jvm->DetachCurrentThread() < 0)
    {
      WEBRTC_TRACE(kTraceWarning, kTraceVideoCapture, 0,
                  "  Could not detach thread from JVM");
    }
  }

  WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
    "%s: width=%d height=%d", __FUNCTION__, width, height);
    return true;
}

void ScreenCapturerAndroid::Start(Callback* callback) {
  assert(!callback_);
  WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0, "%s ", __FUNCTION__);
  callback_ = callback;
}

bool ScreenCapturerAndroid::GetScreenList(ScreenList* screens) {
    return false;
}
    
  // Select the screen to be captured. Returns false in case of a failure (e.g.
  // if there is no screen with the specified id). If this is never called, the
  // full desktop is captured.
bool ScreenCapturerAndroid::SelectScreen(ScreenId id) {
    return false;
}

    
void ScreenCapturerAndroid::Capture(const DesktopRegion& region) {
  if(!g_javaScreenCaptureClass) {
      WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0, "[ScreenCapturerAndroid]: g_javaScreenCaptureClass hasn't be set.");
  }


  if (!activity_) {
      return;
  }
  // get the JNI env for this thread
  JNIEnv *env = NULL;
  bool isAttached = false;
  
   // get the JNI env for this thread
  if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
  {
     // try to attach the thread and get the env
     // Attach this thread to JVM
    jint res = g_jvm->AttachCurrentThread(&env, NULL);
    if ((res < 0) || !env)
    {
      WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
                    "%s: Could not attach thread to JVM (%d, %p)", __FUNCTION__, res, env);
      return ;
    }
    isAttached = true;
  }
  
   // get the method ID
    jmethodID captureScreenMethodID = env->GetMethodID(g_javaScreenCaptureClass,
                                                  "CaptureScreen", "()V");
    if(!captureScreenMethodID) {
        WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
        "[ScreenCapturerAndroid]: g_javaScreenCaptureObject %p, screenRectMethodID %p, activity_ %p, this %p, (jlong)this: %d", g_javaScreenCaptureObject, captureScreenMethodID, activity_, this, (jlong)this);
    }
    env->CallVoidMethod(g_javaScreenCaptureObject, captureScreenMethodID);
    
    // Detach this thread if it was attached
    if (isAttached)
    {
      if (g_jvm->DetachCurrentThread() < 0)
      {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoCapture, 0,
            "%s: Could not detach thread from JVM", __FUNCTION__);
      }
    }
}

void ScreenCapturerAndroid::SetScreenShareActivity(void * activity)
{
  if(!activity) {
      WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
                "[ScreenCapturerAndroid]: Set Screen Share Activity error, activity is null");
      return;
  }
  activity_ = activity;
  WEBRTC_TRACE(kTraceError, kTraceVideoCapture, 0,
              "[ScreenCapturerAndroid]: Set Screen Share Activity Done.");
}

    
//yuv image data
void JNICALL ScreenCapturerAndroid::ProvideScreeData(JNIEnv * env,
                                                    jobject,
                                                    jobject obj,
                                                    jbyteArray javaDesktopFrame,
                                                    jint length,
                                                    jint width,
                                                    jint height,
													jlong context) {
    if(  ScreenCapturerAndroid::current_obj_prt == NULL) {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture,
              -1, "%s: current screent capturer android obj is null.", __FUNCTION__);
        return;
    }

    ScreenCapturerAndroid* captureModule = ScreenCapturerAndroid::current_obj_prt;
    if(!captureModule) {
      WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture,
              -1, "captureModule ptr is null.");
    }

    if(captureModule->callback_) {
        yuntongxunwebrtc::scoped_ptr<BasicDesktopFrame> frame(new BasicDesktopFrame(DesktopSize(width, height)));
        if (!frame.get()) {
            WEBRTC_TRACE(yuntongxunwebrtc::kTraceWarning, yuntongxunwebrtc::kTraceVideoCapture, -1, "%s: frame.get() failed.", __FUNCTION__);
            captureModule->callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
            return;
        }

        jbyte* desktopFrame = env->GetByteArrayElements(javaDesktopFrame, NULL);
        memcpy((frame.get())->data(), desktopFrame, length);
        captureModule->callback_->OnCaptureCompleted(frame.release(), kCapture_Ok, NULL);
        env->ReleaseByteArrayElements(javaDesktopFrame, desktopFrame, JNI_ABORT);
    }
}

// static
ScreenCapturer* ScreenCapturer::Create() {
  return new ScreenCapturerAndroid();
}

}  // namespace yuntongxunwebrtc