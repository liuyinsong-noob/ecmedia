//
//  video_capture_iOS_info.h
//  video_capture
//
//  Created by Lee Sean on 13-2-4.
//
//
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
#import "video_capture_OSX.h"
@class VideoCaptureOSXObjC;
@class ECOSXCaptureCCP;

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        
        class VideoCaptureOSXInfo
        {
        public:
            VideoCaptureOSXInfo();
            ~VideoCaptureOSXInfo();
            int registerOwner(cloopenwebrtc::videocapturemodule::VideoCaptureOSX* owner);
            int registerOwner(int i);
            int setCaptureDeviceById(char* uniqueId);
            int setCaptureHeightAndWidthAndFrameRate(int height,int width,int frameRate);
#ifdef __APPLE_CC__
            int setCaptureRotate(VideoCaptureRotation rotate);
#endif
            int startCapture();
            int stopCapture();
            int setLocalVieoView(void* view);
            int updateLossRate(int lossRate);
        private:
            ECOSXCaptureCCP *videoCaptureiOSObjc;
        };
    }
}
#endif
