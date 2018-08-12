//
//  video_capture_iOS.h
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//

//#import <Foundation/Foundation.h>

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_H_

#include <stdio.h>

#include "../../video_capture_impl.h"
#include "video_capture_iOS_utility.h"
#include "../../device_info_impl.h"


// Forward declaraion
//@class VideoCaptureiOSObjC;
//@class VideoCaptureiOSDeviceInfoObjC;

namespace yuntongxunwebrtc
{
    namespace videocapturemodule
    {
        class VideoCaptureiOSDeviceInfo;
        class VideoCaptureiOSInfo;
        VideoCaptureiOSDeviceInfo* myCreateDeviceInfo(const WebRtc_Word32 id);
        class VideoCaptureiOS : public VideoCaptureImpl
        {
        public:
            VideoCaptureiOS(const WebRtc_Word32 id);
            virtual ~VideoCaptureiOS();
            
            /*
             *   Create a video capture module object
             *
             *   id - unique identifier of this video capture module object
             *   deviceUniqueIdUTF8 -  name of the device. Available names can be found
             *       by using GetDeviceName
             *   deviceUniqueIdUTF8Length - length of deviceUniqueIdUTF8
             */
            static void Destroy(VideoCaptureModule* module);
            
            WebRtc_Word32 Init(const WebRtc_Word32 id,
                               const char* deviceUniqueIdUTF8);
            
            
            // Start/Stop
            virtual WebRtc_Word32 StartCapture(const VideoCaptureCapability& capability);
            virtual WebRtc_Word32 StopCapture();
#ifdef __APPLE_CC__
            virtual WebRtc_Word32 SetCaptureRotation(VideoCaptureRotation rotation);
#endif
            // Properties of the set device
            
            virtual bool CaptureStarted();
            
            WebRtc_Word32 CaptureSettings(VideoCaptureCapability& settings);
//            VideoCaptureiOSDeviceInfo *CreateDeviceInfo(const WebRtc_Word32 id);
            
            virtual WebRtc_Word32 SetPreviewWindow(void* window);
            
            virtual WebRtc_Word32 UpdateLossRate(int lossRate);
            virtual void setBeautyFace(bool enable);
            virtual void setVideoFilter(ECImageFilterType filterType);
        protected:
            // Help functions
            WebRtc_Word32 SetCameraOutput();
            
        private:
            
            
//            VideoCaptureiOSObjC*        _captureDevice;
            VideoCaptureiOSInfo*          _captureDevice;
            VideoCaptureiOSDeviceInfo*    _captureInfo;
//            VideoCaptureiOSDeviceInfoObjC*    _captureInfo;
            bool                    _isCapturing;
            WebRtc_Word32            _id;
            WebRtc_Word32            _captureWidth;
            WebRtc_Word32            _captureHeight;
            WebRtc_Word32            _captureFrameRate;
            char                     _currentDeviceNameUTF8[MAX_NAME_LENGTH];
            char                     _currentDeviceUniqueIdUTF8[MAX_NAME_LENGTH];
            char                     _currentDeviceProductUniqueIDUTF8[MAX_NAME_LENGTH];
            WebRtc_Word32            _frameCount;
        };
    }  // namespace videocapturemodule
}  // namespace yuntongxunwebrtc

#endif
