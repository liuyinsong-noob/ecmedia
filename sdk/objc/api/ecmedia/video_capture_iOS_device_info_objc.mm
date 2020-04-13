//
//  video_capture_device_info_objc.m
//  video_capture
//
//  Created by yukening on 2020/3/12.
//  Copyright © 2020 yukening. All rights reserved.
//
//

#import <UIKit/UIKit.h>
#ifdef TARGET_OS_IPHONE
#import <AVFoundation/AVFoundation.h>

#import "video_capture_iOS_device_info_objc.h"


@interface ECVideoCaptureiOSDeviceInfoObjC (Private)
- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;
@end

@implementation ECVideoCaptureiOSDeviceInfoObjC

@synthesize captureDevicesInfo = _captureDevicesInfo;
- (id)init
{
    self = [super init];
    if (nil != self)
    {
        [self checkOSSupported];
        [self initializeVariables];
    }
    else
    {
        return nil;
    }
    return self;
}

- (void)dealloc
{
    self.captureDevicesInfo = nil;
#if ! __has_feature(objc_arc)
    [super dealloc];
#endif
}


#pragma mark - public methods

- (NSNumber *)displayCaptureSettingsDialogBoxWithDevice:(const char *)deviceUniqueIdUTF8 AndTitle:(const char *)dialogTitleUTF8 AndParentWindow:(void *)parentWindow AtX:(uint32_t)positionX AndY:(uint32_t)positionY
{
//    NSString* strTitle = [NSString stringWithFormat:@"%s", dialogTitleUTF8];
//    NSString* strButton = @"Alright";
//    NSString* strMessage = [NSString stringWithFormat:@"Device %s is capturing", deviceUniqueIdUTF8];
//    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:strTitle message:strMessage delegate:nil cancelButtonTitle:strButton otherButtonTitles: nil];
//    [alert show];
//    [alert release];
    return [NSNumber numberWithInt:0];
}


- (NSNumber *)getCaptureDeviceCount
{
    [self getCaptureDevices];
    return [NSNumber numberWithInt:_captureDeviceCountInfo];
}

- (NSNumber *)getDeviceNamesFromIndex:(uint32_t)index DefaultName:(char *)deviceName WithLength:(uint32_t)deviceNameLength AndUniqueID:(char *)deviceUniqueID WithLength:(uint32_t)deviceUniqueIDLength AndProductID:(char *)deviceProductID WithLength:(uint32_t)deviceProductIDLength
{
    if(NO == _OSSupportedInfo)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(index > (uint32_t)_captureDeviceCountInfo)
    {
        return [NSNumber numberWithInt:-1];
    }
//    NSLog(@"getCaptureDevices self=%d  info=%d", (int)self, (int)_captureDevicesInfo);
//    NSLog(@"getCaptureDevices=%d %@", (int)_captureDevicesInfo, _captureDevicesInfo);
    
    AVCaptureDevice* tempCaptureDevice =
    (AVCaptureDevice*)[_captureDevicesInfo objectAtIndex:index];
    if(!tempCaptureDevice)
    {
        return [NSNumber numberWithInt:-1];
    }
    
    memset(deviceName, 0, deviceNameLength);
    memset(deviceUniqueID, 0, deviceUniqueIDLength);
    
    bool successful = NO;
    
    NSString* tempString = [tempCaptureDevice localizedName];
    successful = [tempString getCString:(char*)deviceName
                              maxLength:deviceNameLength encoding:NSUTF8StringEncoding];
    if(NO == successful)
    {
        memset(deviceName, 0, deviceNameLength);
        return [NSNumber numberWithInt:-1];
    }
    
    tempString = [tempCaptureDevice uniqueID];
    successful = [tempString getCString:(char*)deviceUniqueID
                              maxLength:deviceUniqueIDLength encoding:NSUTF8StringEncoding];
    if(NO == successful)
    {
        memset(deviceUniqueID, 0, deviceUniqueIDLength);
        return [NSNumber numberWithInt:-1];
    }
    
    return [NSNumber numberWithInt:0];
}


#pragma mark - 

- (NSNumber *)initializeVariables
{
    if(NO == _OSSupportedInfo)
    {
        return [NSNumber numberWithInt:0];
    }
    
//    _poolInfo = [[NSAutoreleasePool alloc]init];
    _captureDeviceCountInfo = 0;
    [self getCaptureDevices];
    
    return [NSNumber numberWithInt:0];
}

- (void)checkOSSupported
{
    _OSSupportedInfo = YES;
}


- (NSNumber *)getCaptureDevices
{
    if(NO == _OSSupportedInfo)
    {
        return [NSNumber numberWithInt:0];
    }
   
    if(_captureDeviceCountInfo > 0 )
      return [NSNumber numberWithInt:_captureDeviceCountInfo];
    #if defined(WEBRTC_IOS) && defined(__IPHONE_10_0) && \
        __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
      AVCaptureDeviceDiscoverySession *session = [AVCaptureDeviceDiscoverySession
          discoverySessionWithDeviceTypes:@[ AVCaptureDeviceTypeBuiltInWideAngleCamera ]
                                mediaType:AVMediaTypeVideo
                                 position:AVCaptureDevicePositionUnspecified];
       self.captureDevicesInfo =  session.devices;
    #else
       self.captureDevicesInfo =  [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    #endif
    
    _captureDeviceCountInfo = _captureDevicesInfo.count;
    if(_captureDeviceCountInfo < 1){
        return [NSNumber numberWithInt:0];
    }
    return [NSNumber numberWithInt:_captureDeviceCountInfo];
}













@end
#endif
