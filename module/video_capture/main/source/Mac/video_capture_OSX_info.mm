//
//  video_capture_iOS_info.m
//  video_capture
//
//  Created by Lee Sean on 13-2-4.
//
//
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
#import "video_capture_OSX_info.h"
#import "video_capture_OSX_ObjC.h"
#import "osxcapture.h"

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        VideoCaptureOSXInfo::VideoCaptureOSXInfo()
        {
            videoCaptureiOSObjc = [[ECOSXCaptureCCP alloc] init];
        }
        VideoCaptureOSXInfo::~VideoCaptureOSXInfo()
        {
            [videoCaptureiOSObjc release];
            videoCaptureiOSObjc = nil;
        }
        //int                    registerOwner(VideoCaptureiOS* owner);
        int VideoCaptureOSXInfo::registerOwner(cloopenwebrtc::videocapturemodule::VideoCaptureOSX *owner)
        {
            return [[videoCaptureiOSObjc registerOwner:owner] intValue];
        }
        
        int VideoCaptureOSXInfo::setCaptureDeviceById(char* uniqueId)
        {
            return [[videoCaptureiOSObjc setCaptureDeviceById:uniqueId] intValue];
        }
        
        int VideoCaptureOSXInfo::setCaptureHeightAndWidthAndFrameRate(int height,int width,int frameRate)
        {
            return [[videoCaptureiOSObjc setCaptureHeight:height AndWidth:width AndFrameRate:frameRate] intValue];
        }
        
        int VideoCaptureOSXInfo::startCapture()
        {

//            dispatch_async(dispatch_get_main_queue(), ^{
//                [videoCaptureiOSObjc startCapture];
//            });
//            return 0;

            [videoCaptureiOSObjc performSelectorOnMainThread:@selector(startCapture) withObject:nil waitUntilDone:NO];
            return 0;//[[videoCaptureiOSObjc startCapture] intValue];

        }
#ifdef __APPLE_CC__
        int VideoCaptureOSXInfo::setCaptureRotate(VideoCaptureRotation rotate)
        {
//            [videoCaptureiOSObjc setCaptureRotate:rotate];
            return 0;
        }
#endif
        int VideoCaptureOSXInfo::stopCapture()
        {
            [videoCaptureiOSObjc performSelectorOnMainThread:@selector(stopCapture) withObject:nil waitUntilDone:NO];
            return 0;//[[videoCaptureiOSObjc stopCapture] intValue];
        }
        
        int VideoCaptureOSXInfo::setLocalVieoView(void* view)
        {
            [videoCaptureiOSObjc performSelectorOnMainThread:@selector(setParentView:) withObject:(NSView*)view waitUntilDone:NO];
            return 0;
        }
        
        int VideoCaptureOSXInfo::updateLossRate(int lossRate)
        {
            [videoCaptureiOSObjc updateLossRate:lossRate];
            return 0;
        }
    }
}
#endif
