//
//  video_capture_iOS_info.h
//  video_capture
//  Created by yukening on 2020/3/12.
//  Copyright © 2020 yukening. All rights reserved.
//
//

#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_INFO_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_INFO_H_
#import "video_capture_iOS_device_info_objc.h"
#include "modules/video_capture/device_info_impl.h"
#include "modules/video_capture/video_capture.h"

class VideoCaptureiOSDeviceInfo:  public webrtc::VideoCaptureModule::DeviceInfo
{
public:
    
  static VideoCaptureiOSDeviceInfo* GetInstance();
 virtual ~VideoCaptureiOSDeviceInfo();
    
    int32_t Init();
    
    virtual uint32_t NumberOfDevices();
    
    /*
     * Returns the available capture devices.
     * deviceNumber   -[in] index of capture device
     * deviceNameUTF8 - friendly name of the capture device
     * deviceUniqueIdUTF8 - unique name of the capture device if it exist.
     *      Otherwise same as deviceNameUTF8
     * productUniqueIdUTF8 - unique product id if it exist. Null terminated
     *      otherwise.
     */
    virtual int32_t GetDeviceName(
                                  uint32_t deviceNumber, char* deviceNameUTF8,
                                  uint32_t deviceNameLength, char* deviceUniqueIdUTF8,
                                  uint32_t deviceUniqueIdUTF8Length,
                                  char* productUniqueIdUTF8 = 0,
                                  uint32_t productUniqueIdUTF8Length = 0);
    
    /*
     *   Returns the number of capabilities for this device
     */
    virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);
    
    /*
     *   Gets the capabilities of the named device
     */
    virtual int32_t GetCapability(
                                  const char* deviceUniqueIdUTF8,
                                  const uint32_t deviceCapabilityNumber,
                                  webrtc::VideoCaptureCapability& capability);
    
    /*
     *  Gets the capability that best matches the requested width, height and frame rate.
     *  Returns the deviceCapabilityNumber on success.
     */
    virtual int32_t GetBestMatchedCapability(
                                             const char* deviceUniqueIdUTF8,
                                             const webrtc::VideoCaptureCapability& requested,
                                             webrtc::VideoCaptureCapability& resulting);
    
    /*
     * Display OS /capture device specific settings dialog
     */
    virtual int32_t DisplayCaptureSettingsDialogBox(
                                                    const char* deviceUniqueIdUTF8,
                                                    const char* dialogTitleUTF8, void* parentWindow,
                                                    uint32_t positionX, uint32_t positionY);
    // Gets clockwise angle the captured frames should be rotated in order
     // to be displayed correctly on a normally rotated display.
     virtual int32_t GetOrientation(const char* deviceUniqueIdUTF8,
                                    webrtc::VideoRotation& orientation);
protected:
    virtual int32_t CreateCapabilityMap(
                                        const char* deviceUniqueIdUTF8);
   VideoCaptureiOSDeviceInfo(const int32_t id);
   static VideoCaptureiOSDeviceInfo* m_pInstance;
    
    ECVideoCaptureiOSDeviceInfoObjC*    _captureInfo;
};

#endif
