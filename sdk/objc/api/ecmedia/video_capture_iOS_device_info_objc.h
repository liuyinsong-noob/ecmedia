//
//  video_capture_device_info_objc.h
//  video_capture
//
//  Created by yukening on 2020/3/12.
//  Copyright © 2020 yukening. All rights reserved.
//
//

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_DEVICE_INFO_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_DEVICE_INFO_OBJC_H_
#import <Foundation/Foundation.h>

@interface ECVideoCaptureiOSDeviceInfoObjC : NSObject{
    bool                                _OSSupportedInfo;
    NSArray*                            _captureDevicesInfo;
//    NSAutoreleasePool*                    _poolInfo;
    int                                    _captureDeviceCountInfo;
    
}

@property (retain) NSArray*                            captureDevicesInfo;

- (NSNumber*)getCaptureDeviceCount;

- (NSNumber*)getDeviceNamesFromIndex:(uint32_t)index
                         DefaultName:(char*)deviceName
                          WithLength:(uint32_t)deviceNameLength
                         AndUniqueID:(char*)deviceUniqueID
                          WithLength:(uint32_t)deviceUniqueIDLength
                        AndProductID:(char*)deviceProductID
                          WithLength:(uint32_t)deviceProductIDLength;

- (NSNumber*)displayCaptureSettingsDialogBoxWithDevice:
(const char*)deviceUniqueIdUTF8
                                              AndTitle:(const char*)dialogTitleUTF8
                                       AndParentWindow:(void*) parentWindow AtX:(uint32_t)positionX
                                                  AndY:(uint32_t) positionY;
@end

#endif
