/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#include "trace.h"
#include "../../video_capture_config.h"
#import "video_capture_qtkit_info_objc.h"

#include "video_capture.h"

namespace yuntongxunwebrtc
{
namespace videocapturemodule
{

VideoCaptureMacQTKitInfo::VideoCaptureMacQTKitInfo(const WebRtc_Word32 id) :
    DeviceInfoImpl(id)
{
    _captureInfo = [[VideoCaptureMacQTKitInfoObjC alloc] init];
}

VideoCaptureMacQTKitInfo::~VideoCaptureMacQTKitInfo()
{
    [_captureInfo release];

}

WebRtc_Word32 VideoCaptureMacQTKitInfo::Init()
{

    return 0;
}

WebRtc_UWord32 VideoCaptureMacQTKitInfo::NumberOfDevices()
{

    WebRtc_UWord32 captureDeviceCount =
        [[_captureInfo getCaptureDeviceCount]intValue];
    return captureDeviceCount;

}

WebRtc_Word32 VideoCaptureMacQTKitInfo::GetDeviceName(
    WebRtc_UWord32 deviceNumber, char* deviceNameUTF8,
    WebRtc_UWord32 deviceNameLength, char* deviceUniqueIdUTF8,
    WebRtc_UWord32 deviceUniqueIdUTF8Length, char* productUniqueIdUTF8,
    WebRtc_UWord32 productUniqueIdUTF8Length)
{
    int errNum = [[_captureInfo getDeviceNamesFromIndex:deviceNumber
                   DefaultName:deviceNameUTF8 WithLength:deviceNameLength
                   AndUniqueID:deviceUniqueIdUTF8
                   WithLength:deviceUniqueIdUTF8Length
                   AndProductID:productUniqueIdUTF8
                   WithLength:productUniqueIdUTF8Length]intValue];
    return errNum;
}

WebRtc_Word32 VideoCaptureMacQTKitInfo::NumberOfCapabilities(
    const char* deviceUniqueIdUTF8)
{
    return 5;
//    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
//    // "analog". QTKit will do it's best to convert frames to what ever format
//    // you ask for.
//    WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, _id,
//                 "NumberOfCapabilities is not supported on the Mac platform.");
//    return -1;
}


WebRtc_Word32 VideoCaptureMacQTKitInfo::GetCapability(
    const char* deviceUniqueIdUTF8,
    const WebRtc_UWord32 deviceCapabilityNumber,
    VideoCaptureCapability& capability)
{
    
    switch (deviceCapabilityNumber) {
        case 0:
        {
            capability.width = 240; //288;
            capability.height = 320; //352;
            capability.maxFPS = 15;
        }
            break;
        case 1:
        {
            capability.width = 288; //288;
            capability.height = 352; //352;
            capability.maxFPS = 15;
        }
            break;
        case 2:
        {
            capability.width = 480; //288;
            capability.height = 640; //352;
            capability.maxFPS = 15;
        }
            break;
        case 3:
        {
            capability.width = 540; //288;
            capability.height = 960; //352;
            capability.maxFPS = 15;
        }
            break;
        case 4:
        {
            capability.width = 720;
            capability.height = 1280;
            capability.maxFPS = 15;
        }
            break;
        default:
            break;
    }
    
    return 0;
//    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
//    // "analog". QTKit will do it's best to convert frames to what ever format
//    // you ask for.
//    WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, _id,
//                 "NumberOfCapabilities is not supported on the Mac platform.");
//    return -1;
}


WebRtc_Word32 VideoCaptureMacQTKitInfo::GetBestMatchedCapability(
    const char*deviceUniqueIdUTF8,
    const VideoCaptureCapability& requested, VideoCaptureCapability& resulting)
{
    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
    // "analog". QTKit will do it's best to convert frames to what ever format
    // you ask for.
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideoCapture, _id,
                 "NumberOfCapabilities is not supported on the Mac platform.");
    return -1;
}

WebRtc_Word32 VideoCaptureMacQTKitInfo::DisplayCaptureSettingsDialogBox(
    const char* deviceUniqueIdUTF8,
    const char* dialogTitleUTF8, void* parentWindow,
    WebRtc_UWord32 positionX, WebRtc_UWord32 positionY)
{

    return [[_captureInfo
             displayCaptureSettingsDialogBoxWithDevice:deviceUniqueIdUTF8
             AndTitle:dialogTitleUTF8
             AndParentWindow:parentWindow AtX:positionX AndY:positionY]
             intValue];
}

WebRtc_Word32 VideoCaptureMacQTKitInfo::CreateCapabilityMap(
    const char* deviceUniqueIdUTF8)
{
    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
    // "analog". QTKit will do it's best to convert frames to what ever format
    // you ask for.
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideoCapture, _id,
                 "NumberOfCapabilities is not supported on the Mac platform.");
     return -1;
}
}  // namespace videocapturemodule
}  // namespace webrtc
#endif
