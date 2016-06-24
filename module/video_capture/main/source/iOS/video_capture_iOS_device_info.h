//
//  video_capture_iOS_info.h
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_INFO_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_iOS_VIDEO_CAPTURE_iOS_INFO_H_

#include "../../video_capture_impl.h"
#include "../../device_info_impl.h"
#include "video_capture_iOS_utility.h"

#include "map_wrapper.h"
//#import "video_capture_qtkit_info_objc.h"
#import "video_capture_iOS_device_info_objc.h"

//@class VideoCaptureiOSDeviceInfoObjC;

namespace cloopenwebrtc
{
    namespace videocapturemodule
    {
        
        class VideoCaptureiOSDeviceInfo: public DeviceInfoImpl
        {
        public:
            
            VideoCaptureiOSDeviceInfo(const WebRtc_Word32 id);
            virtual ~VideoCaptureiOSDeviceInfo();
            
            WebRtc_Word32 Init();
            
            virtual WebRtc_UWord32 NumberOfDevices();
            
            /*
             * Returns the available capture devices.
             * deviceNumber   -[in] index of capture device
             * deviceNameUTF8 - friendly name of the capture device
             * deviceUniqueIdUTF8 - unique name of the capture device if it exist.
             *      Otherwise same as deviceNameUTF8
             * productUniqueIdUTF8 - unique product id if it exist. Null terminated
             *      otherwise.
             */
            virtual WebRtc_Word32 GetDeviceName(
                                                WebRtc_UWord32 deviceNumber, char* deviceNameUTF8,
                                                WebRtc_UWord32 deviceNameLength, char* deviceUniqueIdUTF8,
                                                WebRtc_UWord32 deviceUniqueIdUTF8Length,
                                                char* productUniqueIdUTF8 = 0,
                                                WebRtc_UWord32 productUniqueIdUTF8Length = 0);
            
            /*
             *   Returns the number of capabilities for this device
             */
            virtual WebRtc_Word32 NumberOfCapabilities(
                                                       const char* deviceUniqueIdUTF8);
            
            /*
             *   Gets the capabilities of the named device
             */
            virtual WebRtc_Word32 GetCapability(
                                                const char* deviceUniqueIdUTF8,
                                                const WebRtc_UWord32 deviceCapabilityNumber,
                                                VideoCaptureCapability& capability);
            
            /*
             *  Gets the capability that best matches the requested width, height and frame rate.
             *  Returns the deviceCapabilityNumber on success.
             */
            virtual WebRtc_Word32 GetBestMatchedCapability(
                                                           const char* deviceUniqueIdUTF8,
                                                           const VideoCaptureCapability& requested,
                                                           VideoCaptureCapability& resulting);
            
            /*
             * Display OS /capture device specific settings dialog
             */
            virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
                                                                  const char* deviceUniqueIdUTF8,
                                                                  const char* dialogTitleUTF8, void* parentWindow,
                                                                  WebRtc_UWord32 positionX, WebRtc_UWord32 positionY);
            
        protected:
            virtual WebRtc_Word32 CreateCapabilityMap(
                                                      const char* deviceUniqueIdUTF8);
            
            ECVideoCaptureiOSDeviceInfoObjC*    _captureInfo;
        };
    }  // namespace videocapturemodule
}  // namespace cloopenwebrtc

#endif
