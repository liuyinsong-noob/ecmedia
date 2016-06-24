//
//  video_capture_iOS_info.m
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#import "video_capture_iOS_device_info.h"
#import "video_capture_iOS_device_info_objc.h"
#include "trace.h"
#include "../../video_capture_config.h"
//#import "video_capture_iOS_device_info_objc.h"
//#import "video_capture_qtkit_info_objc.h"

#include "video_capture.h"

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        VideoCaptureiOSDeviceInfo::VideoCaptureiOSDeviceInfo(const WebRtc_Word32 id) :
        DeviceInfoImpl(id)
        {
            _captureInfo = [[ECVideoCaptureiOSDeviceInfoObjC alloc] init];
        }
        
        VideoCaptureiOSDeviceInfo::~VideoCaptureiOSDeviceInfo()
        {
            [_captureInfo release];
            
        }
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::Init()
        {
            
            return 0;
        }
        
        WebRtc_UWord32 VideoCaptureiOSDeviceInfo::NumberOfDevices()
        {
            
            WebRtc_UWord32 captureDeviceCount =
            [[_captureInfo getCaptureDeviceCount] intValue];
            return captureDeviceCount;
            
        }
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::GetDeviceName(
                                                              WebRtc_UWord32 deviceNumber, char* deviceNameUTF8,
                                                              WebRtc_UWord32 deviceNameLength, char* deviceUniqueIdUTF8,
                                                              WebRtc_UWord32 deviceUniqueIdUTF8Length, char* productUniqueIdUTF8,
                                                              WebRtc_UWord32 productUniqueIdUTF8Length)
        {
            int errNum= [[_captureInfo getDeviceNamesFromIndex:deviceNumber
                                                    DefaultName:deviceNameUTF8 WithLength:deviceNameLength
                                                    AndUniqueID:deviceUniqueIdUTF8
                                                     WithLength:deviceUniqueIdUTF8Length
                                                   AndProductID:productUniqueIdUTF8
                                                     WithLength:productUniqueIdUTF8Length]intValue];
            return errNum;
        }
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::NumberOfCapabilities(
                                                                     const char* deviceUniqueIdUTF8)
        {
            return 5;
        }
        
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::GetCapability(
                                                              const char* deviceUniqueIdUTF8,
                                                              const WebRtc_UWord32 deviceCapabilityNumber,
                                                              VideoCaptureCapability& capability)
        {
            // Not implemented. Mac doesn't use discrete steps in capabilities, rather
            // "analog". QTKit will do it's best to convert frames to what ever format
            // you ask for.
//            WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, _id,
//                         "NumberOfCapabilities is not supported on the Mac platform.");
//            return -1;
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
        }
        
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::GetBestMatchedCapability(
                                                                         const char*deviceUniqueIdUTF8,
                                                                         const VideoCaptureCapability& requested, VideoCaptureCapability& resulting)
        {
            // Not implemented. Mac doesn't use discrete steps in capabilities, rather
            // "analog". QTKit will do it's best to convert frames to what ever format
            // you ask for.
            WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideoCapture, _id,
                         "NumberOfCapabilities is not supported on the Mac platform.");
            return -1;
        }
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::DisplayCaptureSettingsDialogBox(
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
        
        WebRtc_Word32 VideoCaptureiOSDeviceInfo::CreateCapabilityMap(
                                                                    const char* deviceUniqueIdUTF8)
        {
            // Not implemented. Mac doesn't use discrete steps in capabilities, rather
            // "analog". QTKit will do it's best to convert frames to what ever format
            // you ask for.
            WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideoCapture, _id,
                         "NumberOfCapabilities is not supported on the Mac platform.");
            return -1;
        }
    }  // namespace videocapturemodule
}  // namespace cloopenwebrtc
