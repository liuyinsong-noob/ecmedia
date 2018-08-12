//
//  VideoCaptureiOSObjC.h
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_OBJC_H_
#import <Foundation/Foundation.h>
#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreData/CoreData.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/NSLock.h>

#import "video_capture_recursive_lock.h"

#include "video_capture_OSX.h"
//#include "msvideo.h"

class CriticalSectionWrapper;
@interface VideoCaptureOSXObjC : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>{
//    BOOL                            _capturing;
//    int                             _counter;
//    int                             _frameRate;
//    int                             _frameWidth;
//    int                             _frameHeight;
      int                             _framesDelivered;
//    int                             _framesRendered;
    BOOL                            _OSSupported;
    BOOL                            _captureInitialized;
        
    //WebRTC Custom classes
    yuntongxunwebrtc::videocapturemodule::VideoCaptureOSX* _owner;
//    VideoCaptureRecursiveLock*            _rLock;
    
    //iOS variables
    AVCaptureSession*                    _captureSession;
    AVCaptureDeviceInput*                _captureVideoDeviceInput;
    AVCaptureVideoDataOutput*           _captureVideoOutput;
    NSArray*                            _captureDevices;
    int                                    _captureDeviceCount;
    int                                    _captureDeviceIndex;
    NSString*                            _captureDeviceName;
    char                                _captureDeviceNameUTF8[1024];
    char                                _captureDeviceNameUniqueID[1024];
    char                                _captureDeviceNameProductID[1024];
//    NSString*                            _key;
//    NSNumber*                            _val;
//    NSDictionary*                        _videoSettings;
//    NSString*                            _captureQuality;
    //本地视频
    AVCaptureVideoPreviewLayer*         previewLayer;
    NSView*                             localVideoWindow;
    AVCaptureVideoOrientation _videoOrientation;
    
    // other
    dispatch_queue_t                    videoDataOutputQueue;
    
    
//    //yuv data handle
    int              _yuvDataSize;
    unsigned char*   _yuvDataBuf;
    unsigned char*   _scaleDataBuf;
    
//    int                     _yuvWidth;
//    int                     _yuvHeight;
//    int                     _yPitch;
//    int                     _uvPitch;
    NSConditionLock *_lock;
//    MSPicture* _curPic;
    NSThread *handleCaptureDataThread;
}

@property (nonatomic, retain) NSView *localVideoWindow;

- (NSNumber*)registerOwner:(yuntongxunwebrtc::videocapturemodule::VideoCaptureOSX*)owner;
- (NSNumber*)setCaptureDeviceById:(char*)uniqueId;
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate;
- (NSNumber*)startCapture;
- (NSNumber*)stopCapture;
@end

#endif
