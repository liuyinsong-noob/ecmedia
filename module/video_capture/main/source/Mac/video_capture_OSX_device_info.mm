//
//  video_capture_iOS_info.m
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
#import "video_capture_OSX_device_info.h"
#import "video_capture_OSX_device_info_objc.h"
#include "trace.h"
#include "../../video_capture_config.h"
//#import "video_capture_iOS_device_info_objc.h"
//#import "video_capture_qtkit_info_objc.h"

#include "video_capture.h"

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        VideoCaptureOSXDeviceInfo::VideoCaptureOSXDeviceInfo(const WebRtc_Word32 id) :
        DeviceInfoImpl(id)
        {
            _captureInfo = [[ECVideoCaptureOSXDeviceInfoObjC alloc] init];
        }
        
        VideoCaptureOSXDeviceInfo::~VideoCaptureOSXDeviceInfo()
        {
            [_captureInfo release];
            
        }
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::Init()
        {
            
            return 0;
        }
        
        WebRtc_UWord32 VideoCaptureOSXDeviceInfo::NumberOfDevices()
        {
            
            WebRtc_UWord32 captureDeviceCount =
            [[_captureInfo getCaptureDeviceCount] intValue];
            return captureDeviceCount;
            
        }
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::GetDeviceName(
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
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::NumberOfCapabilities(
                                                                     const char* deviceUniqueIdUTF8)
        {
            return [_captureInfo getDeviceCapCount:[NSString stringWithUTF8String:deviceUniqueIdUTF8]].intValue;
        }
        
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::GetCapability(
                                                              const char* deviceUniqueIdUTF8,
                                                              const WebRtc_UWord32 deviceCapabilityNumber,
                                                              VideoCaptureCapability& capability)
        {
            capability.width = [_captureInfo getDeviceSpecifiedCapFactor:[NSString stringWithUTF8String:deviceUniqueIdUTF8] WithCapID:deviceCapabilityNumber Width:0].intValue;
            capability.height = [_captureInfo getDeviceSpecifiedCapFactor:[NSString stringWithUTF8String:deviceUniqueIdUTF8] WithCapID:deviceCapabilityNumber Width:1].intValue;
            capability.maxFPS = [_captureInfo getDeviceSpecifiedCapFactor:[NSString stringWithUTF8String:deviceUniqueIdUTF8] WithCapID:deviceCapabilityNumber Width:2].intValue;

            return 0;
        }
        
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::GetBestMatchedCapability(
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
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::DisplayCaptureSettingsDialogBox(
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
        
        WebRtc_Word32 VideoCaptureOSXDeviceInfo::CreateCapabilityMap(
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
#endif
