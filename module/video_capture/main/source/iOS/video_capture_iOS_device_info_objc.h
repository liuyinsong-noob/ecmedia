//
//  video_capture_device_info_objc.h
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//

//
//  video_capture_qtkit_info_objc.h
//
//

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_DEVICE_INFO_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_DEVICE_INFO_OBJC_H_
#import <Foundation/Foundation.h>
#include "video_capture_iOS_utility.h"
//#include "video_capture_qtkit_info.h"

#include "../../video_capture_impl.h"
#include "../../device_info_impl.h"

@interface ECVideoCaptureiOSDeviceInfoObjC : NSObject{
    bool                                _OSSupportedInfo;
    NSArray*                            _captureDevicesInfo;
//    NSAutoreleasePool*                    _poolInfo;
    int                                    _captureDeviceCountInfo;
    
}

@property (retain) NSArray*                            captureDevicesInfo;

- (NSNumber*)getCaptureDeviceCount;

- (NSNumber*)getDeviceNamesFromIndex:(WebRtc_UWord32)index
                         DefaultName:(char*)deviceName
                          WithLength:(WebRtc_UWord32)deviceNameLength
                         AndUniqueID:(char*)deviceUniqueID
                          WithLength:(WebRtc_UWord32)deviceUniqueIDLength
                        AndProductID:(char*)deviceProductID
                          WithLength:(WebRtc_UWord32)deviceProductIDLength;

- (NSNumber*)displayCaptureSettingsDialogBoxWithDevice:
(const char*)deviceUniqueIdUTF8
                                              AndTitle:(const char*)dialogTitleUTF8
                                       AndParentWindow:(void*) parentWindow AtX:(WebRtc_UWord32)positionX
                                                  AndY:(WebRtc_UWord32) positionY;
@end

#endif