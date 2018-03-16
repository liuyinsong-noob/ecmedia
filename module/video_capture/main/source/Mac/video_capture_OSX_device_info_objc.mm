//
//  video_capture_device_info_objc.m
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>

#import "video_capture_OSX_device_info_objc.h"
#include "trace.h"


using namespace cloopenwebrtc;

@interface ECVideoCaptureOSXDeviceInfoObjC (Private)
- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;
@end

@implementation ECVideoCaptureOSXDeviceInfoObjC

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

- (NSNumber *)displayCaptureSettingsDialogBoxWithDevice:(const char *)deviceUniqueIdUTF8 AndTitle:(const char *)dialogTitleUTF8 AndParentWindow:(void *)parentWindow AtX:(WebRtc_UWord32)positionX AndY:(WebRtc_UWord32)positionY
{
    NSString* strTitle = [NSString stringWithFormat:@"%s", dialogTitleUTF8];
    NSString* strButton = @"Alright";
    NSString* strMessage = [NSString stringWithFormat:@"Device %s is capturing", deviceUniqueIdUTF8];
//    NSAlert *alert = [[NSAlert alloc] initWithTitle:strTitle message:strMessage delegate:nil cancelButtonTitle:strButton otherButtonTitles: nil];
//    [alert show];
//    [alert release];
    return [NSNumber numberWithInt:0];
}


- (NSNumber *)getCaptureDeviceCount
{
    [self getCaptureDevices];
    return [NSNumber numberWithInt:_captureDeviceCountInfo];
}

- (NSNumber *)getDeviceCapCount:(NSString *)deviceUniqueID
{
    if (self.captureDevicesInfo.count <= 0) {
        return [NSNumber numberWithInt:-1];
    }
    NSInteger count = 0;
    for (id cursor in self.captureDevicesInfo) {
        AVCaptureDevice *tempDevcie = cursor;
        if ([tempDevcie.uniqueID isEqualToString:deviceUniqueID]) {
            count = tempDevcie.formats.count;
            break;
        }
    }
    return [NSNumber numberWithInteger:count];
    
}

//
//WebRtc_Word32 VideoCaptureOSXDeviceInfo::GetCapability(
//                                                       const char* deviceUniqueIdUTF8,
//                                                       const WebRtc_UWord32 deviceCapabilityNumber,
//                                                       VideoCaptureCapability& capability)




- (NSNumber *)getDeviceSpecifiedCapFactor:(NSString *)deviceUniqueID WithCapID:(WebRtc_UWord32)capID Width:(NSInteger)flag
{
    int ret = 0;
    if (self.captureDevicesInfo.count <= 0) {
        return [NSNumber numberWithInt:-1];
    }
    AVCaptureDevice *device = nil;
    BOOL found = NO;
    for (id cursor in _captureDevicesInfo) {
        device = cursor;
        if ([device.uniqueID isEqualToString:deviceUniqueID]) {
            found = YES;
            break;
        }
    }
    if (found) {
        NSArray *capArr = device.formats;
        if (capID <= capArr.count) {
            AVCaptureDeviceFormat *format = capArr[capID];
            CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            NSArray *rangearr = device.activeFormat.videoSupportedFrameRateRanges;
            if (0 == flag) {
                ret = dim.width;
            }
            else if (1 == flag)
            {
                ret = dim.height;
            }
            else if (2 == flag)
            {
                AVFrameRateRange *maxFps = rangearr[rangearr.count/2];
                ret = maxFps.minFrameRate;
            }
            else
            {
                NSLog(@"[WARNING] invalid flag %ld, only support 0(width), 1(height), 2(maxFps)", (long)flag);
            }
        }
        else
        {
            NSLog(@"[WARNNING] invalid device capability number!");
            ret = -2;
        }
    }
    else
    {
        NSLog(@"[WARNNING] cannot find the device specified by %@", deviceUniqueID);
        ret = -1;
    }
    return [NSNumber numberWithInt:ret];
    
}

- (NSNumber *)getDeviceNamesFromIndex:(WebRtc_UWord32)index DefaultName:(char *)deviceName WithLength:(WebRtc_UWord32)deviceNameLength AndUniqueID:(char *)deviceUniqueID WithLength:(WebRtc_UWord32)deviceUniqueIDLength AndProductID:(char *)deviceProductID WithLength:(WebRtc_UWord32)deviceProductIDLength
{
    if(NO == _OSSupportedInfo)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(index > (WebRtc_UWord32)_captureDeviceCountInfo)
    {
        return [NSNumber numberWithInt:-1];
    }
    
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
    
//    if(_captureDevicesInfo)
//    {
//        [_captureDevicesInfo release];
//    }
    
    self.captureDevicesInfo = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    _captureDeviceCountInfo = _captureDevicesInfo.count;
    if(_captureDeviceCountInfo < 1){
        return [NSNumber numberWithInt:0];
    }
    return [NSNumber numberWithInt:_captureDeviceCountInfo];
}

@end
#endif
