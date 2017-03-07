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

#include "scoped_ptr.h"
#include "trace.h"

#include <jni.h>
#include <Android/bitmap.h>

#define AndroidJavaScreenCaptureClass "com/yuntongxun/ecsdk/core/voip/ScreenCaptureAndroid"

namespace cloopenwebrtc {

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
                                          jobject bitmap,
                                          jlong context);
public:
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

	// get java capture class type (note path to class packet)	               
	jclass screenCaptureClassLocal = thisEnv->FindClass(AndroidJavaScreenCaptureClass);
	if (!screenCaptureClassLocal) {
	  WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, -1,
	               "%s: could not find java class", __FUNCTION__);
	  return -1;
	}
	// create a global reference to the class
	// (to tell JNI that we are referencing it
	// after this function has returned)
	ScreenCapturerAndroid::g_javaScreenCaptureClass = static_cast<jclass>(thisEnv->NewGlobalRef(screenCaptureClassLocal));
	if (!ScreenCapturerAndroid::g_javaScreenCaptureClass) {
	  WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, -1,
 	               "%s: could not create android screen capture class reference",
	               __FUNCTION__);
	  return -1;
	}
	// Delete local class ref, we only use the global ref
	thisEnv->DeleteLocalRef(screenCaptureClassLocal);
	
	JNINativeMethod nativeFunctions =
	    { "ProvideScreeData", "(Ljava/lang/Object;J)V",
	      (void*) &ScreenCapturerAndroid::ProvideScreeData };
	if (thisEnv->RegisterNatives(ScreenCapturerAndroid::g_javaScreenCaptureClass, &nativeFunctions, 1) == 0) {
	  WEBRTC_TRACE(cloopenwebrtc::kTraceDebug, cloopenwebrtc::kTraceVideoCapture, -1,
	               "%s: Registered native functions", __FUNCTION__);
	}	else {
	  WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, -1,
	               "%s: Failed to register native functions",
	               __FUNCTION__);
	  return -1;
	}

  // get the method ID for the void(void) constructor
  jmethodID cid = thisEnv->GetMethodID(ScreenCapturerAndroid::g_javaScreenCaptureClass, "<init>", "()V");
  if (cid == NULL) {
       	  WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, -1,
	               "%s: Failed to get init method.",
	               __FUNCTION__);
    return -1;
  }

  // construct the object
  jobject javaObjLocal = thisEnv->NewObject(ScreenCapturerAndroid::g_javaScreenCaptureClass, cid);
  if (!javaObjLocal)
  {
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, 0,
                 "%s: could not create Java sc object", __FUNCTION__);
    return -1;
  }

  ScreenCapturerAndroid::g_javaScreenCaptureObject = thisEnv->NewGlobalRef(javaObjLocal);
  if (!ScreenCapturerAndroid::g_javaScreenCaptureObject)
  {
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, 0,
                 "%s: could not create Java sc object reference",
                 __FUNCTION__);
    return -1;
  }
	// Delete local object ref, we only use the global ref
	thisEnv->DeleteLocalRef(javaObjLocal);
	
}
  
ScreenCapturerAndroid::ScreenCapturerAndroid()
    : callback_(NULL){
  
}

ScreenCapturerAndroid::~ScreenCapturerAndroid() {

}

bool ScreenCapturerAndroid::IsAeroEnabled() {
  bool result = false;
  return result;
}

bool  ScreenCapturerAndroid::GetShareCaptureRect(int &width, int &height)
{    
  
  WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, 0,
               "%s begin.", __FUNCTION__);

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
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, 0,
                   "  Could not attach thread to JVM (%d, %p)", res, env);
      return -1;
    }
    isAttached = true;
  }

  // get the method ID
  jmethodID screenRectMethodID = env->GetMethodID(g_javaScreenCaptureClass,
                                                  "GetScreenRect", "(Ljava/lang/Object;)[I");

  // Call java object method
    jintArray screenRect = (jintArray)env->CallObjectMethod( g_javaScreenCaptureObject, screenRectMethodID, activity_);

   jint *rect;  
   rect = env->GetIntArrayElements(screenRect, false);  
   if(rect == NULL) {  
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
      WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, 0,
                   "  Could not detach thread from JVM");
    }
  }

  WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, 0,
    "%s: width=%d height=%d", __FUNCTION__, width, height);

    return true;
}

void ScreenCapturerAndroid::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, 0,
             "%s ", __FUNCTION__);

  callback_ = callback;
}

bool ScreenCapturerAndroid::GetScreenList(ScreenList* screens)
    {
        return false;
    }
    
    // Select the screen to be captured. Returns false in case of a failure (e.g.
    // if there is no screen with the specified id). If this is never called, the
    // full desktop is captured.
bool ScreenCapturerAndroid::SelectScreen(ScreenId id)
    {
        return false;
    }

    
void ScreenCapturerAndroid::Capture(const DesktopRegion& region) {

  WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, 0,
      "%s: benhur test", __FUNCTION__);

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
       WEBRTC_TRACE(kTraceError, kTraceAudioDevice, 0,
                    "%s:  Could not attach thread to JVM (%d, %p)", __FUNCTION__, res, env);
       return ;
     }
     isAttached = true;
   }
  
   // get the method ID
   jmethodID captureScreenMethodID = env->GetMethodID(g_javaScreenCaptureClass,
                                                   "CaptureScreen", "(Ljava/lang/Object;J)V");
  
   env->CallVoidMethod( g_javaScreenCaptureObject, captureScreenMethodID, activity_, (jlong)this);
    
   // Detach this thread if it was attached
   if (isAttached)
   {
     if (g_jvm->DetachCurrentThread() < 0)
     {
       WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, 0,
           "%s: Could not detach thread from JVM", __FUNCTION__);
     }
   }

  //callback_->OnCaptureCompleted(/*frame.release()*/, kCapture_Ok, NULL);
}

void ScreenCapturerAndroid::SetScreenShareActivity(void * activity)
{
    activity_ = activity;
}

void JNICALL ScreenCapturerAndroid::ProvideScreeData(JNIEnv * env,
                                          jobject,
                                          jobject bitmap,
                                          jlong context)
{
    ScreenCapturerAndroid* captureModule = reinterpret_cast<ScreenCapturerAndroid*>(context);
               
    if(captureModule && captureModule->callback_) {
    
        AndroidBitmapInfo bmpInfo={0};
        if( AndroidBitmap_getInfo(env,bitmap,&bmpInfo) < 0 ) {
            captureModule->callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
            return ;
         }
        unsigned char* dataFromBmp=NULL;
        if(AndroidBitmap_lockPixels(env,bitmap,(void**)&dataFromBmp) < 0)
        {   
            captureModule->callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
            return ;
        }
        
        cloopenwebrtc::scoped_ptr<BasicDesktopFrame> frame(new BasicDesktopFrame(DesktopSize(bmpInfo.width, bmpInfo.height)));
        if (!frame.get()) {
            captureModule->callback_->OnCaptureCompleted(NULL, kCapture_NoCaptureImage);
            return;
        }

        WEBRTC_TRACE(cloopenwebrtc::kTraceWarning, cloopenwebrtc::kTraceVideoCapture,
                   -1, "%s: IncomingFrame width:%d height:%d format:%d stride:%d.", 
                   __FUNCTION__, bmpInfo.width, bmpInfo.height, bmpInfo.format, bmpInfo.stride);
        if(bmpInfo.format == ANDROID_BITMAP_FORMAT_RGB_565)
            memcpy((frame.get())->data(), dataFromBmp, bmpInfo.width*bmpInfo.height*2);
        else if(bmpInfo.format == ANDROID_BITMAP_FORMAT_RGBA_8888)
            memcpy((frame.get())->data(), dataFromBmp, bmpInfo.width*bmpInfo.height*4);

        captureModule->callback_->OnCaptureCompleted(frame.release(), kCapture_Ok, NULL);

        AndroidBitmap_unlockPixels(env,bitmap);
    }
}

// static
ScreenCapturer* ScreenCapturer::Create() {
  return new ScreenCapturerAndroid();
}

}  // namespace cloopenwebrtc

