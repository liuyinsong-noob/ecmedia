//
//  video_capture_iOS_info.h
//  video_capture
//
//  Created by Lee Sean on 13-2-4.
//
//
#import "video_capture_iOS.h"
@class VideoCaptureiOSObjC;
@class ECIOSCaptureCCP;

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        
        class VideoCaptureiOSInfo
        {
        public:
            VideoCaptureiOSInfo();
            ~VideoCaptureiOSInfo();
            int registerOwner(cloopenwebrtc::videocapturemodule::VideoCaptureiOS* owner);
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
            void setBeautyFace(bool enable);
            void setVideoFilter(ECImageFilterType filterType);
        private:
            ECIOSCaptureCCP *videoCaptureiOSObjc;
        };
    }
}
