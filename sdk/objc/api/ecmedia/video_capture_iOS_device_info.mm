//
//  video_capture_iOS_info.m
//  video_capture
//
//  Created by yukening on 2020/3/12.
//  Copyright © 2020 yukening. All rights reserved.
//
//
#import "video_capture_iOS_device_info.h"
#import "video_capture_iOS_device_info_objc.h"

VideoCaptureiOSDeviceInfo* VideoCaptureiOSDeviceInfo::m_pInstance = NULL;
VideoCaptureiOSDeviceInfo* VideoCaptureiOSDeviceInfo::GetInstance(){
  m_pInstance = NULL;
    if (m_pInstance == NULL) {
       m_pInstance = new VideoCaptureiOSDeviceInfo(0);
     }
     return m_pInstance;
}
VideoCaptureiOSDeviceInfo::VideoCaptureiOSDeviceInfo(const int32_t id) 
{
    _captureInfo = [[ECVideoCaptureiOSDeviceInfoObjC alloc] init];
}

VideoCaptureiOSDeviceInfo::~VideoCaptureiOSDeviceInfo()
{
   // [_captureInfo release];
}

int32_t VideoCaptureiOSDeviceInfo::Init()
{
    
    return 0;
}

uint32_t VideoCaptureiOSDeviceInfo::NumberOfDevices()
{
    
    uint32_t captureDeviceCount =
    [[_captureInfo getCaptureDeviceCount] intValue];
    return captureDeviceCount;
    
}

int32_t VideoCaptureiOSDeviceInfo::GetDeviceName(
                                                 uint32_t deviceNumber, char* deviceNameUTF8,
                                                 uint32_t deviceNameLength, char* deviceUniqueIdUTF8,
                                                 uint32_t deviceUniqueIdUTF8Length, char* productUniqueIdUTF8,
                                                 uint32_t productUniqueIdUTF8Length)
{
    int errNum= [[_captureInfo getDeviceNamesFromIndex:deviceNumber
                                           DefaultName:deviceNameUTF8 WithLength:deviceNameLength
                                           AndUniqueID:deviceUniqueIdUTF8
                                            WithLength:deviceUniqueIdUTF8Length
                                          AndProductID:productUniqueIdUTF8
                                            WithLength:productUniqueIdUTF8Length]intValue];
    return errNum;
}

int32_t VideoCaptureiOSDeviceInfo::NumberOfCapabilities(const char* deviceUniqueIdUTF8)
{
    return 4;
}


int32_t VideoCaptureiOSDeviceInfo::GetCapability(
                                                 const char* deviceUniqueIdUTF8,
                                                 const uint32_t deviceCapabilityNumber,
                                                 webrtc::VideoCaptureCapability& capability)
{
    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
    // "analog". QTKit will do it's best to convert frames to what ever format
    // you ask for.
    //            WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, _id,
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


int32_t VideoCaptureiOSDeviceInfo::GetBestMatchedCapability(
                                                            const char*deviceUniqueIdUTF8,
                                                            const webrtc::VideoCaptureCapability& requested, webrtc::VideoCaptureCapability& resulting)
{
    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
    // "analog". QTKit will do it's best to convert frames to what ever format
    // you ask for.
    return -1;
}

int32_t VideoCaptureiOSDeviceInfo::DisplayCaptureSettingsDialogBox(
                                                                   const char* deviceUniqueIdUTF8,
                                                                   const char* dialogTitleUTF8, void* parentWindow,
                                                                   uint32_t positionX, uint32_t positionY)
{
    
    return [[_captureInfo
             displayCaptureSettingsDialogBoxWithDevice:deviceUniqueIdUTF8
             AndTitle:dialogTitleUTF8
             AndParentWindow:parentWindow AtX:positionX AndY:positionY]
            intValue];
}

int32_t VideoCaptureiOSDeviceInfo::CreateCapabilityMap(
                                                             const char* deviceUniqueIdUTF8)
{
    // Not implemented. Mac doesn't use discrete steps in capabilities, rather
    // "analog". QTKit will do it's best to convert frames to what ever format
    // you ask for.
    return -1;
}

 int32_t VideoCaptureiOSDeviceInfo::GetOrientation(const char* deviceUniqueIdUTF8,
                                                   webrtc::VideoRotation& orientation){
   return 0;
 }
