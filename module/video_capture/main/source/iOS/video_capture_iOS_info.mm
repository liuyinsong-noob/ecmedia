//
//  video_capture_iOS_info.m
//  video_capture
//
//  Created by Lee Sean on 13-2-4.
//
//
#import "video_capture_iOS_info.h"
#import "video_capture_iOS_ObjC.h"
#import "ioscapture.h"

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        VideoCaptureiOSInfo::VideoCaptureiOSInfo()
        {
            videoCaptureiOSObjc = [[ECIOSCaptureCCP alloc] init];
        }
        VideoCaptureiOSInfo::~VideoCaptureiOSInfo()
        {
            [videoCaptureiOSObjc release];
            videoCaptureiOSObjc = nil;
        }
        //int                    registerOwner(VideoCaptureiOS* owner);
        int VideoCaptureiOSInfo::registerOwner(cloopenwebrtc::videocapturemodule::VideoCaptureiOS *owner)
        {
            return [[videoCaptureiOSObjc registerOwner:owner] intValue];
        }
        
        int VideoCaptureiOSInfo::setCaptureDeviceById(char* uniqueId)
        {
            return [[videoCaptureiOSObjc setCaptureDeviceById:uniqueId] intValue];
        }
        
        int VideoCaptureiOSInfo::setCaptureHeightAndWidthAndFrameRate(int height,int width,int frameRate)
        {
            return [[videoCaptureiOSObjc setCaptureHeight:height AndWidth:width AndFrameRate:frameRate] intValue];
        }
        
        int VideoCaptureiOSInfo::startCapture()
        {

//            dispatch_async(dispatch_get_main_queue(), ^{
//                [videoCaptureiOSObjc startCapture];
//            });
//            return 0;

            [videoCaptureiOSObjc performSelectorOnMainThread:@selector(startCapture) withObject:nil waitUntilDone:NO];
            return 0;//[[videoCaptureiOSObjc startCapture] intValue];

        }
#ifdef __APPLE_CC__
        int VideoCaptureiOSInfo::setCaptureRotate(VideoCaptureRotation rotate)
        {
            [videoCaptureiOSObjc setCaptureRotate:rotate];
            return 0;
        }
#endif
        int VideoCaptureiOSInfo::stopCapture()
        {
            [videoCaptureiOSObjc performSelectorOnMainThread:@selector(stopCapture) withObject:nil waitUntilDone:NO];
            return 0;//[[videoCaptureiOSObjc stopCapture] intValue];
        }
        
        int VideoCaptureiOSInfo::setLocalVieoView(void* view)
        {
            [videoCaptureiOSObjc performSelectorOnMainThread:@selector(setParentView:) withObject:(UIView*)view waitUntilDone:NO];
            return 0;
        }
        
        int VideoCaptureiOSInfo::updateLossRate(int lossRate)
        {
            [videoCaptureiOSObjc updateLossRate:lossRate];
            return 0;
        }
    }
}
