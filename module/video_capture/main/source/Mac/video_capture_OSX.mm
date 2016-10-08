//
//  video_capture_iOS.m
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
#import "video_capture_OSX.h"
#import "video_capture_OSX_info.h"
#import "video_capture_OSX_device_info.h"
#include "ref_count.h"
#include "trace.h"
#include "critical_section_wrapper.h"
#include "../../video_capture_config.h"
namespace cloopenwebrtc
{
    
    namespace videocapturemodule
    {

		VideoCaptureModule* VideoCaptureImpl::Create(
		                                             const WebRtc_Word32 id,
		                                             const char* deviceUniqueIdUTF8,
                                                     VideoCaptureCapability *settings)
		{
		
		    RefCountImpl<VideoCaptureOSX>* implementation =
		    new RefCountImpl<VideoCaptureOSX>(id);
		    
		    if (!implementation || implementation->Init(id, deviceUniqueIdUTF8) != 0) {
		        delete implementation;
		        implementation = NULL;
		    }
		    return implementation;
		
		
		}

		VideoCaptureModule::DeviceInfo* VideoCaptureImpl::CreateDeviceInfo(
		                                                                   const WebRtc_Word32 id) {
		   return new VideoCaptureOSXDeviceInfo(id);
		}

        VideoCaptureOSX::VideoCaptureOSX(const WebRtc_Word32 id) :
        VideoCaptureImpl(id),
        _captureDevice(NULL),
        _captureInfo(NULL),
        _isCapturing(false),
        _id(id),
        _captureWidth(QTKIT_DEFAULT_WIDTH),
        _captureHeight(QTKIT_DEFAULT_HEIGHT),
        _captureFrameRate(QTKIT_DEFAULT_FRAME_RATE),
        _frameCount(0)
        {
            
            memset(_currentDeviceNameUTF8, 0, MAX_NAME_LENGTH);
            memset(_currentDeviceUniqueIdUTF8, 0, MAX_NAME_LENGTH);
            memset(_currentDeviceProductUniqueIDUTF8, 0, MAX_NAME_LENGTH);
        }
        
        VideoCaptureOSX::~VideoCaptureOSX()
        {
            
            WEBRTC_TRACE(cloopenwebrtc::kTraceDebug, cloopenwebrtc::kTraceVideoCapture, _id,
                         "~VideoCaptureMacQTKit() called");
            if(_captureDevice)
            {
                _captureDevice->registerOwner((cloopenwebrtc::videocapturemodule::VideoCaptureOSX *)nil);
//                [_captureDevice stopCapture];
//                [_captureDevice release];
//                _captureDevice->stopCapture();
                delete _captureDevice;
            }
            
            if(_captureInfo)
            {
//                [_captureInfo release];
                delete _captureInfo;
            }
        }
        
        WebRtc_Word32 VideoCaptureOSX::Init(
                                            const WebRtc_Word32 id, const char* iDeviceUniqueIdUTF8)
        {
            CriticalSectionScoped cs(&_apiCs);
            
            
            const WebRtc_Word32 nameLength =
            (WebRtc_Word32) strlen((char*)iDeviceUniqueIdUTF8);
            if(nameLength>kVideoCaptureUniqueNameLength)
                return -1;
            
            // Store the device name
            _deviceUniqueId = new char[nameLength+1];
            memcpy(_deviceUniqueId, iDeviceUniqueIdUTF8,nameLength+1);
            
//            _captureDevice = [[VideoCaptureOSXObjC alloc] init];
            _captureDevice = new VideoCaptureOSXInfo();
            if(NULL == _captureDevice)
            {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, id,
                             "Failed to create an instance of "
                             "VideoCaptureMacQTKitObjC");
                return -1;
            }
            
//            if(-1 == [[_captureDevice registerOwner:this] intValue])
            if(-1 == _captureDevice->registerOwner(this))
            {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, id,
                             "Failed to register owner for _captureDevice");
                return -1;
            }
            
            if(0 == strcmp((char*)iDeviceUniqueIdUTF8, ""))
            {
                // the user doesn't want to set a capture device at this time
                return 0;
            }
            
//            _captureInfo = [[VideoCaptureOSXDeviceInfoObjC alloc]init];
            _captureInfo = new VideoCaptureOSXDeviceInfo(0);
            if(NULL == _captureInfo)
            {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, id, "Failed to create an instance of VideoCaptureMacQTKitInfoObjC");
                return -1;
            }
            
//            int captureDeviceCount = [[_captureInfo getCaptureDeviceCount] intValue];
            int captureDeviceCount = _captureInfo->NumberOfDevices();
            if(captureDeviceCount < 0)
            {
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, id,
                             "No Capture Devices Present");
                return -1;
            }
            
            const int NAME_LENGTH = 1024;
            char deviceNameUTF8[1024] = "";
            char deviceUniqueIdUTF8[1024] = "";
            char deviceProductUniqueIDUTF8[1024] = "";
            
            bool captureDeviceFound = false;
            for(int index = 0; index < captureDeviceCount; index++){
                
                memset(deviceNameUTF8, 0, NAME_LENGTH);
                memset(deviceUniqueIdUTF8, 0, NAME_LENGTH);
                memset(deviceProductUniqueIDUTF8, 0, NAME_LENGTH);
//                if(-1 == [[_captureInfo getDeviceNamesFromIndex:index
//                                                    DefaultName:deviceNameUTF8 WithLength:NAME_LENGTH
//                                                    AndUniqueID:deviceUniqueIdUTF8 WithLength:NAME_LENGTH
//                                                   AndProductID:deviceProductUniqueIDUTF8
//                                                     WithLength:NAME_LENGTH]intValue])
                if(-1 == _captureInfo->GetDeviceName(index,deviceNameUTF8,NAME_LENGTH,deviceUniqueIdUTF8,NAME_LENGTH,deviceProductUniqueIDUTF8,NAME_LENGTH))
                {
                    WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, _id,
                                 "GetDeviceName returned -1 for index %d", index);
                    return -1;
                }
                if(0 == strcmp((const char*)iDeviceUniqueIdUTF8,
                               (char*)deviceUniqueIdUTF8))
                {
                    // we have a match
                    captureDeviceFound = true;
                    break;
                }
            }
            
            if(false == captureDeviceFound)
            {
                WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideoCapture, _id,
                             "Failed to find capture device unique ID %s",
                             iDeviceUniqueIdUTF8);
                return -1;
            }
            
            // at this point we know that the user has passed in a valid camera. Let's
            // set it as the current.
//            if(-1 == [[_captureDevice
//                       setCaptureDeviceById:(char*)deviceUniqueIdUTF8]intValue])
            if(-1 == _captureDevice->setCaptureDeviceById((char *)deviceUniqueIdUTF8))
            {
                strcpy((char*)_deviceUniqueId, (char*)deviceUniqueIdUTF8);
                WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCapture, _id,
                             "Failed to set capture device %s (unique ID %s) even "
                             "though it was a valid return from "
                             "VideoCaptureMacQTKitInfo");
                return -1;
            }
            
            WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideoCapture, _id,
                         "successfully Init VideoCaptureMacQTKit" );
            return 0;
        }
        
        WebRtc_Word32 VideoCaptureOSX::StartCapture(
                                                    const VideoCaptureCapability& capability)
        {
            
            _captureWidth = capability.width;
            _captureHeight = capability.height;
            _captureFrameRate = capability.maxFPS;
            
//            if(-1 == [[_captureDevice setCaptureHeight:_captureHeight
//                                              AndWidth:_captureWidth AndFrameRate:_captureFrameRate]intValue])
            if(-1 == _captureDevice->setCaptureHeightAndWidthAndFrameRate(_captureHeight, _captureWidth, _captureFrameRate))
            {
                WEBRTC_TRACE(cloopenwebrtc::kTraceInfo, cloopenwebrtc::kTraceVideoCapture, _id,
                             "Could not set width=%d height=%d frameRate=%d",
                             _captureWidth, _captureHeight, _captureFrameRate);
                return -1;
            }
            
//            if(-1 == [[_captureDevice startCapture]intValue])
            if(-1 == _captureDevice->startCapture())
            {
                return -1;
            }
            _isCapturing = true;
            return 0;
        }
        
        WebRtc_Word32 VideoCaptureOSX::StopCapture()
        {
//            [_captureDevice stopCapture];
            _captureDevice->stopCapture();
            
            _isCapturing = false;
            return 0;
        }
        
        WebRtc_Word32 VideoCaptureOSX::UpdateLossRate(int lossRate)
        {
            return _captureDevice->updateLossRate(lossRate);
        }
        
        bool VideoCaptureOSX::CaptureStarted()
        {
            return _isCapturing;
        }
        
        WebRtc_Word32 VideoCaptureOSX::CaptureSettings(VideoCaptureCapability& settings)
        {
            settings.width = _captureWidth;
            settings.height = _captureHeight;
            settings.maxFPS = _captureFrameRate;
            return 0;
        }
        
        
        WebRtc_Word32 VideoCaptureOSX::SetPreviewWindow(void* window)
        {            
            _captureDevice->setLocalVieoView(window);
            return 0;
        }
        
//        VideoCaptureOSXDeviceInfo *VideoCaptureOSX::CreateDeviceInfo(const WebRtc_Word32 id)
//        {
//            return new VideoCaptureOSXDeviceInfo(id);
//        }
        
        
        // ********** begin functions inherited from DeviceInfoImpl **********
        
        struct VideoCaptureCapabilityMacQTKit:public VideoCaptureCapability
        {
            VideoCaptureCapabilityMacQTKit()
            {
            }
        };
        
#ifdef __APPLE_CC__
        WebRtc_Word32 VideoCaptureOSX::SetCaptureRotation(VideoCaptureRotation rotation)
        {
            VideoCaptureImpl::SetCaptureRotation(rotation);
            _captureDevice->setCaptureRotate(rotation);
            return 0;
        }
#endif

    }  // namespace videocapturemodule
}
#endif
