//
//  VideoCaptureiOSObjC.m
//  video_capture
//
//  Created by Lee Sean on 13-1-8.
//
//
#define DEFAULT_CAPTURE_DEVICE_INDEX    1
#define DEFAULT_FRAME_RATE              30
#define DEFAULT_FRAME_WIDTH             240;//352
#define DEFAULT_FRAME_HEIGHT            320;//288
#define ROTATE_CAPTURED_FRAME           1
#define LOW_QUALITY                     1

#import "video_capture_OSX_ObjC.h"
#include "video_capture_OSX_utility.h"
//#include "third_party/libyuv/include/libyuv.h"

#include "trace.h"


#include "msvideo.h"

extern NSView *globalLocalVideo;


using namespace yuntongxunwebrtc;
using namespace videocapturemodule;

static void capture_queue_cleanup(void* p) {
	VideoCaptureOSXObjC *capture = (VideoCaptureOSXObjC *)p;
	[capture release];
}

@interface VideoCaptureOSXObjC (Private)
- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVideoCapture;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;
- (void)deviceOrientationDidChange;
@end

@implementation VideoCaptureOSXObjC
@synthesize localVideoWindow;

- (id)init
{
    self = [super init];
    if (nil != self) {
        [self checkOSSupported];
        [self initializeVariables];
        [self deviceOrientationDidChange];
    }
    else
    {
        return nil;
    }
    return self;
}

- (void)dealloc
{
//    if(testYuv)
//        fclose(testYuv);
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    if (_captureSession) {
        [_captureSession stopRunning];
        [_captureSession release];
        
        // Will free the queue
        [_captureVideoOutput setSampleBufferDelegate:nil queue:nil];
    }
//    [_rLock release];
    [_captureVideoOutput release];
    [_captureDeviceName release];
    self.localVideoWindow = nil;
    
    if(_yuvDataBuf)
        free(_yuvDataBuf);
    
    [super dealloc];
}


#pragma mark - public methods

- (NSNumber *)registerOwner:(yuntongxunwebrtc::videocapturemodule::VideoCaptureOSX *)owner
{
    if (!owner) {
        return [NSNumber numberWithInt:-1];
    }
    _owner = owner;
    return [NSNumber numberWithInt:0];
}

- (NSNumber *)setCaptureDeviceById:(char *)uniqueId
{
    NSLog(@"setCaptureDeviceById =%s", uniqueId);
    if(NO == _OSSupported)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d OS version does not support necessary APIs",
                     __FUNCTION__, __LINE__);
        return [NSNumber numberWithInt:0];
    }
    
    if(!uniqueId || (0 == strcmp("", uniqueId)))
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d  \"\" was passed in for capture device name",
                     __FUNCTION__, __LINE__);
        memset(_captureDeviceNameUTF8, 0, 1024);
        return [NSNumber numberWithInt:0];
    }
    
    if(0 == strcmp(uniqueId, _captureDeviceNameUniqueID))
    {
        // camera already set
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d Capture device is already set to %s", __FUNCTION__,
                     __LINE__, _captureDeviceNameUTF8);
        return [NSNumber numberWithInt:0];
    }
    
    bool success = NO;
    AVCaptureDevice* tempCaptureDevice;
    for(int index = 0; index < _captureDeviceCount; index++)
    {
        tempCaptureDevice = (AVCaptureDevice*)[_captureDevices
                                               objectAtIndex:index];
        char tempCaptureDeviceId[1024] = "";
        [[tempCaptureDevice uniqueID]
         getCString:tempCaptureDeviceId maxLength:1024
         encoding:NSUTF8StringEncoding];
        if(0 == strcmp(uniqueId, tempCaptureDeviceId))
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                         "%s:%d Found capture device id %s as index %d",
                         __FUNCTION__, __LINE__, tempCaptureDeviceId, index);
            success = YES;
            [[tempCaptureDevice localizedName]
             getCString:_captureDeviceNameUTF8
             maxLength:1024
             encoding:NSUTF8StringEncoding];
            [[tempCaptureDevice uniqueID]
             getCString:_captureDeviceNameUniqueID
             maxLength:1024
             encoding:NSUTF8StringEncoding];
            break;
        }
        
    }
    
    if(NO == success)
    {
        // camera not found
        // nothing has been changed yet, so capture device will stay in it's
        // state
        WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                     "%s:%d Capture device id %s was not found in list of "
                     "available devices.", __FUNCTION__, __LINE__, uniqueId);
        return [NSNumber numberWithInt:0];
    }
    
    NSError* error;
    
    _captureVideoDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:tempCaptureDevice error:&error];
    
    if ([_captureSession canAddInput:_captureVideoDeviceInput]) {
        [_captureSession addInput:_captureVideoDeviceInput];
    }
    
    
    WEBRTC_TRACE(kTraceInfo, kTraceVideoCapture, 0,
                 "%s:%d successfully added capture device: %s", __FUNCTION__,
                 __LINE__, _captureDeviceNameUTF8);
    return [NSNumber numberWithInt:0];
}

- (NSNumber *)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate
{
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }
    _frameWidth = width;
    _frameHeight = height;
    _frameRate = frameRate;
    
    // TODO(mflodman) Check fps settings.
    // [_captureDecompressedVideoOutput
    //     setMinimumVideoFrameInterval:(NSTimeInterval)1/(float)_frameRate];
    NSDictionary* captureDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                       [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange/*kCVPixelFormatType_420YpCbCr8BiPlanarFullRange*/],
                                       (id)kCVPixelBufferPixelFormatTypeKey, nil];
    
    [_captureVideoOutput setVideoSettings:captureDictionary];
    [_captureVideoOutput setAlwaysDiscardsLateVideoFrames:YES];
    
    // these methods return type void so there isn't much we can do about
    // checking success
    return [NSNumber numberWithInt:0];
}

- (void)initLocalViewInMainThread
{
    //本地视频
    previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:_captureSession];
	[previewLayer setBackgroundColor:[[UIColor blackColor] CGColor]];
	[previewLayer setVideoGravity:AVLayerVideoGravityResizeAspect];
//	CALayer *rootLayer = [localVideoWindow layer];
    CALayer *rootLayer = [globalLocalVideo layer];
    
	[rootLayer setMasksToBounds:YES];
	[previewLayer setFrame:[rootLayer bounds]];
    
	[rootLayer addSublayer:previewLayer];
}


- (NSNumber *)startCapture
{
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(YES == _capturing)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(NO == _captureInitialized)
    {
        // this should never be called..... it is initialized on class init
        [self initializeVideoCapture];
    }
  
    
    
    [self performSelectorOnMainThread:@selector(initLocalViewInMainThread) withObject:nil waitUntilDone:YES];
    
    
    AVCaptureConnection *videoConnection = NULL;
    
    [_captureSession beginConfiguration];
    
    for ( AVCaptureConnection *connection in [_captureVideoOutput connections] )
    {
        for ( AVCaptureInputPort *port in [connection inputPorts] )
        {
            if ( [[port mediaType] isEqual:AVMediaTypeVideo] )
            {
                videoConnection = connection;                
            }
        }
    }
    
    if([videoConnection isVideoMinFrameDurationSupported])
    {
        NSLog(@"videoConnection isVideoMinFrameDurationSupported");
        [videoConnection setVideoMinFrameDuration:CMTimeMake(1, 12)];
        [videoConnection setVideoMaxFrameDuration:CMTimeMake(1, 12)];
    }

    
    if([videoConnection isVideoOrientationSupported])
    {
        NSLog(@"videoConnection.videoOrientation =%d", videoConnection.videoOrientation);
        [videoConnection setVideoOrientation:AVCaptureVideoOrientationPortrait];
        
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter] addObserver: self selector:   @selector(deviceOrientationDidChange) name: UIDeviceOrientationDidChangeNotification object: nil];
    }
    
    [_captureSession commitConfiguration];
    
    [_captureSession startRunning];
    
    
    _capturing = YES;
    
    return [NSNumber numberWithInt:0];
}

- (NSNumber *)stopCapture
{
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(nil == _captureSession)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(NO == _capturing)
    {
        return [NSNumber numberWithInt:0];
    }
    
    if(YES == _capturing)
    {
        [_captureSession stopRunning];
    }
    
    [[NSNotificationCenter defaultCenter] removeObserver: self];
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    
    _capturing = NO;
    return [NSNumber numberWithInt:0];
}


#pragma mark - private methods
- (NSNumber *)initializeVariables
{
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }
    
    memset(_captureDeviceNameUTF8, 0, 1024);
    _counter = 0;
    _framesDelivered = 0;
    _captureDeviceCount = 0;
    _capturing = NO;
    _captureInitialized = NO;
    _frameRate = DEFAULT_FRAME_RATE;
    _frameWidth = DEFAULT_FRAME_WIDTH;
    _frameHeight = DEFAULT_FRAME_HEIGHT;
    _captureDeviceName = [[NSString alloc] initWithFormat:@""];
    _captureSession = [[AVCaptureSession alloc] init];
    _captureVideoOutput = [[AVCaptureVideoDataOutput alloc] init];
    
    _captureSession.sessionPreset = AVCaptureSessionPreset640x480;//AVCaptureSessionPreset352x288;
    mDownScalingRequired = TRUE;
    dispatch_queue_t videoDataOutputQueue = dispatch_queue_create("VideoDataOutputQueue", DISPATCH_QUEUE_SERIAL);
    dispatch_set_context(videoDataOutputQueue, [self retain]);
    dispatch_set_finalizer_f(videoDataOutputQueue, capture_queue_cleanup);
    
	[_captureVideoOutput setSampleBufferDelegate:self queue:videoDataOutputQueue];
    
    dispatch_release(videoDataOutputQueue);
    
    _yuvWidth = 0;
    _yuvHeight = 0;
    _yPitch = 0;
    _uvPitch = 0;
    
    _yuvDataSize = 0;
    _yuvDataBuf = nil;
    
    mOutputVideoSize.width=MS_VIDEO_SIZE_QVGA_W;
    mOutputVideoSize.height=MS_VIDEO_SIZE_QVGA_H;
    
    [self getCaptureDevices];
    [self initializeVideoCapture];
    
    _scaleDataBuf = (unsigned char *)malloc(240*320*1.5);
        
    return [NSNumber numberWithInt:0];
}

- (void)checkOSSupported
{
    _OSSupported = YES;
}

- (NSNumber *)getCaptureDevices
{
    if(NO == _OSSupported)
    {
        return [NSNumber numberWithInt:0];
    }
    _captureDevices = [AVCaptureDevice devices];
    
    _captureDeviceCount = _captureDevices.count;
    if(_captureDeviceCount < 1)
    {
        return [NSNumber numberWithInt:0];
    }
    return [NSNumber numberWithInt:0];
}

- (NSNumber *)initializeVideoCapture
{
    if(YES == _captureInitialized)
    {
        return [NSNumber numberWithInt:-1];
    }
//    AVCaptureDevice* videoDevice = (AVCaptureDevice*)[_captureDevices objectAtIndex:0];
    
    [_captureVideoOutput setVideoSettings:
     [NSDictionary dictionaryWithObjectsAndKeys:
      [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange/*kCVPixelFormatType_420YpCbCr8BiPlanarFullRange*/],
      (id)kCVPixelBufferPixelFormatTypeKey, nil]];
    
    // TODO(mflodman) Check fps settings.
    //[_captureDecompressedVideoOutput setMinimumVideoFrameInterval:
    //    (NSTimeInterval)1/(float)_frameRate];
    //[_captureDecompressedVideoOutput setAutomaticallyDropsLateVideoFrames:YES];
    
    if ([_captureSession canAddOutput:_captureVideoOutput]) {
        [_captureSession addOutput:_captureVideoOutput];
        NSLog(@"canAddOutput 1 ");
    }
    [[_captureVideoOutput connectionWithMediaType:AVMediaTypeVideo] setEnabled:NO];
    
    _captureInitialized = YES;
    
    return [NSNumber numberWithInt:0];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{   
    if(NO == _OSSupported)
    {
        return;
    }
    
    if(!_owner)
    {
        return;
    }

    _framesDelivered++;    
//    NSLog(@"captureOutput _framesDelivered %d  ", _framesDelivered);
    
    //thread test
//    const int LOCK_FLAGS = 0; // documentation says to pass 0
//    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
//    
//    CVPixelBufferLockBaseAddress(pixelBuffer, LOCK_FLAGS);
//    int frameWidth = CVPixelBufferGetWidth(pixelBuffer);
//    int frameHeight = CVPixelBufferGetHeight(pixelBuffer);
//    int bytesPerRow0  = CVPixelBufferGetWidthOfPlane(pixelBuffer, 0);
//    int bytesPerRow1  = CVPixelBufferGetWidthOfPlane(pixelBuffer, 1);
//    
//    unsigned char* base_address0 = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
//    unsigned char* base_address1 = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,1);
//    
//    OSType formatType = CVPixelBufferGetPixelFormatType(pixelBuffer);
//    CVPixelBufferUnlockBaseAddress(pixelBuffer, LOCK_FLAGS);
//
//    [_yuvCondition lock];
//        
//    if(_yuvWidth != frameWidth  || _yuvHeight != frameHeight)
//    {
//        if(_yuvDataBuf)
//        {
//            free(_yuvDataBuf);
//            _yuvDataBuf = nil;
//        }
//        if(_uvDataBuf)
//        {
//            free(_uvDataBuf);
//            _uvDataBuf = nil;
//        }
//    }
//    int yuvSize = frameWidth * frameHeight*1.5;
//    if(!_yuvDataBuf)
//    {
//        _yuvDataBuf = (unsigned char *)malloc(yuvSize);
//        if(!_yuvDataBuf)
//        {
//            NSLog(@"%s malloc yuv buffer len=%d Failed!", __FUNCTION__, yuvSize);
//            return;
//        }
//    }
//    if(!_uvDataBuf)
//    {
//        _uvDataBuf = (unsigned char *)malloc(yuvSize/2);
//        if(!_uvDataBuf)
//        {
//            NSLog(@"%s malloc yuv buffer len=%d Failed!", __FUNCTION__, yuvSize);
//            return;
//        }
//    }
//    
//    _yuvWidth = frameWidth;
//    _yuvHeight = frameHeight;
//    _yPitch = bytesPerRow0;
//    _uvPitch = bytesPerRow1;
//    memcpy(_yuvDataBuf, base_address0, frameWidth*frameHeight);
//    memcpy(_uvDataBuf, base_address1, frameWidth*frameHeight/2);
//
////    NSLog(@"captureOutput 1");
//    _yuvDataReady = true;
//
//    [_yuvCondition unlock];
    //thread test end
    
    //        int yuvSize = frameWidth * frameHeight*1.5;
    //        if(yuvSize != _yuvDataSize)
    //        {
    //            if(_yuvDataBuf)
    //            {
    //                free(_yuvDataBuf);
    //                _yuvDataBuf = nil;
    //            }
    //            if(_uvDataBuf)
    //            {
    //                free(_uvDataBuf);
    //                _uvDataBuf = nil;
    //            }
    //
    //            _yuvDataSize = yuvSize;
    //        }
    //        if(!_yuvDataBuf)
    //        {
    //            _yuvDataBuf = (unsigned char *)malloc(yuvSize);
    //            if(!_yuvDataBuf)
    //            {
    //                NSLog(@"%s malloc yuv buffer len=%d Failed!", __FUNCTION__, yuvSize);
    //                return;
    //            }
    //        }
    //
    ////        if(!_uvDataBuf)
    ////        {
    ////            _uvDataBuf = (unsigned char *)malloc(yuvSize/2);
    ////            if(!_uvDataBuf)
    ////            {
    ////                NSLog(@"%s malloc yuv buffer len=%d Failed!", __FUNCTION__, yuvSize);
    ////                return;
    ////            }
    ////        }
    ////        _yuvWidth = frameWidth;
    ////        _yuvHeight = frameHeight;
    ////        _yPitch = bytesPerRow0;
    ////        _uvPitch = bytesPerRow1;
    ////        memcpy(_yuvDataBuf, base_address0, frameWidth*frameHeight);
    ////        memcpy(_uvDataBuf, base_address1, frameWidth*frameHeight/2);
    ////        
    ////        NSLog(@"captureOutput 1");
    ////        _yuvDataReady = true;
    ////        
    ////        [_yuvCondition unlock];
    
    
    const int LOCK_FLAGS = 0; // documentation says to pass 0    
    
    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    
    CVReturn status = CVPixelBufferLockBaseAddress(pixelBuffer, LOCK_FLAGS);
    if (kCVReturnSuccess != status) {
        NSLog(@"ERROR:      Locking base address: %i", status);
        sampleBuffer=nil;
        return;
    }
    /*kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange*/
    size_t plane_width = CVPixelBufferGetWidthOfPlane(pixelBuffer, 0);
    size_t plane_height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 0);
    
    
    unsigned char* base_address0 = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
    unsigned char* base_address1 = (unsigned char*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,1);
    
    
    int rotation=0;
    if (![connection isVideoOrientationSupported]) {
        NSLog(@"connection isVideoOrientationSupported");
        switch (mDeviceOrientation) {
            case 0: {
                rotation = 90;
                break;
            }
            case 90: {
                if ([(AVCaptureDevice*)_captureVideoDeviceInput.device position] == AVCaptureDevicePositionBack) {
                    rotation = 180;
                } else {
                    rotation = 0;
                }
                break;
            }
            case 270: {
                if ([(AVCaptureDevice*)_captureVideoDeviceInput.device position] == AVCaptureDevicePositionBack) {
                    rotation = 0;
                } else {
                    rotation = 180;
                }
                break;
            }
            default: NSLog(@"Unsupported device orientation [%i]",mDeviceOrientation);
        }
    }
    
    int factor =mDownScalingRequired?2:1;
    
//    NSLog(@"  sean000000 mOutputVideoSize.width = %d,plane_width = %ld,mOutputVideoSize.height = %d,plane_height = %ld,factor = %d",mOutputVideoSize.width,plane_width,mOutputVideoSize.height,plane_height,factor);
    switch (rotation) {
        case 0:
        case 180:
            if (mOutputVideoSize.width*factor>plane_width || mOutputVideoSize.height*factor>plane_height) {
                NSLog(@"ERROR:      [1]IOS capture discarding frame because wrong dimensions (%d > %ld || %d > %ld)",
                         mOutputVideoSize.width*factor, plane_width,
                         mOutputVideoSize.height*factor, plane_height);
                return;
            }
            break;
        case 90:
        case 270:
            if (mOutputVideoSize.width*factor>plane_height || mOutputVideoSize.height*factor>plane_width) {
                NSLog(@"ERROR:      [2]IOS capture discarding frame because wrong dimensions (%d > %ld || %d > %ld)",
                         mOutputVideoSize.width*factor, plane_height,
                         mOutputVideoSize.height*factor, plane_width);
                return;
            }
            break;
            
        default: NSLog(@"ERROR:      Unsupported device orientation [%i]",mDeviceOrientation);
    }
    
    
//    NSLog(@"roation = %d,mOutputVideoSize.width = %d,mOutputVideoSize.height = %d",rotation,mOutputVideoSize.width,mOutputVideoSize.height);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, LOCK_FLAGS);
    if(_owner)
    {
//        NSLog(@"\n\n\n");
//        NSLog(@"rotation=%d, mOutputVideoSize.width=%d, mOutputVideoSize.height=%d, CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0)=%ld, CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)=%ld, mDownScalingRequired=%d",rotation,mOutputVideoSize.width,mOutputVideoSize.height,CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0), CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1),mDownScalingRequired);
        MSPicture pict = copy_ycbcrbiplanar_to_true_yuv_with_rotation_and_down_scale_by_2(base_address0,                    base_address1, rotation, mOutputVideoSize.width, mOutputVideoSize.height, CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0), CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1), TRUE, mDownScalingRequired);
        VideoFrameI420 videoFrame;
        videoFrame.y_plane = pict.planes[0];
        videoFrame.u_plane = pict.planes[1];
        videoFrame.v_plane = pict.planes[2];
        
        videoFrame.width = pict.w;//240;//frameWidth;
        videoFrame.height = pict.h;//320;//frameHeight;
        videoFrame.y_pitch = pict.strides[0];//240;//bytesPerRow0;
        videoFrame.u_pitch = videoFrame.v_pitch = pict.strides[1];//120;//bytesPerRow1;
        
        NSLog(@"%s 222222\n", __FUNCTION__);
//        seanmodify
        _owner->IncomingFrameI420(videoFrame, 0);
        free(pict.planes[0]);
    }
}

- (void)deviceOrientationDidChange{
    
    UIDeviceOrientation deviceOrientation = [[UIDevice currentDevice] orientation];
    
    AVCaptureVideoOrientation newOrientation;
    
    if (deviceOrientation == UIDeviceOrientationPortrait){
        NSLog(@"deviceOrientationDidChange - Portrait");
        newOrientation = AVCaptureVideoOrientationPortrait;
        mDeviceOrientation = 0;
    }
    else if (deviceOrientation == UIDeviceOrientationPortraitUpsideDown){
        NSLog(@"deviceOrientationDidChange - UpsideDown");
        newOrientation = AVCaptureVideoOrientationPortraitUpsideDown;
        mDeviceOrientation = 180;
    }
    
    // AVCapture and UIDevice have opposite meanings for landscape left and right (AVCapture orientation is the same as UIInterfaceOrientation)
    else if (deviceOrientation == UIDeviceOrientationLandscapeLeft){
        NSLog(@"deviceOrientationDidChange - LandscapeLeft");
        newOrientation = AVCaptureVideoOrientationLandscapeRight;
        mDeviceOrientation = 90;
    }
    else if (deviceOrientation == UIDeviceOrientationLandscapeRight){
        NSLog(@"deviceOrientationDidChange - LandscapeRight");
        newOrientation = AVCaptureVideoOrientationLandscapeLeft;
        mDeviceOrientation = 270;
    }
    
    else if (deviceOrientation == UIDeviceOrientationUnknown){
//        NSLog(@"deviceOrientationDidChange - Unknown ");
        newOrientation = AVCaptureVideoOrientationPortrait;
        mDeviceOrientation = 0;
    }
    
    else{
        NSLog(@"deviceOrientationDidChange - Face Up or Down");
        newOrientation = AVCaptureVideoOrientationPortrait;
        mDeviceOrientation = 0;
    }
    
    if(_videoOrientation != newOrientation)
    {
        AVCaptureConnection *videoConnection = NULL;
        
        [_captureSession beginConfiguration];
        
        for ( AVCaptureConnection *connection in [_captureVideoOutput connections] )
        {
            for ( AVCaptureInputPort *port in [connection inputPorts] )
            {
                if ( [[port mediaType] isEqual:AVMediaTypeVideo] )
                {
                    videoConnection = connection;
                }
            }
        }
        
        if([videoConnection isVideoOrientationSupported])
        {
            NSLog(@"videoConnection.videoOrientation =%d _videoOrientation=%d", videoConnection.videoOrientation, _videoOrientation);
            [videoConnection setVideoOrientation:newOrientation];
        }
        
//        NSLog(@"  sean000000 mOutputVideoSize.width = %d,mOutputVideoSize.height = %d",mOutputVideoSize.width,mOutputVideoSize.height);
        
        if (newOrientation == 1 || newOrientation == 2) {
            mOutputVideoSize.width = MS_VIDEO_SIZE_QVGA_H;
            mOutputVideoSize.height = MS_VIDEO_SIZE_QVGA_W;
        }
        else if (newOrientation == 3 || newOrientation == 4)
        {
            mOutputVideoSize.width=MS_VIDEO_SIZE_QVGA_W;
            mOutputVideoSize.height=MS_VIDEO_SIZE_QVGA_H;
        }
//         NSLog(@"  sean111111 mOutputVideoSize.width = %d,mOutputVideoSize.height = %d",mOutputVideoSize.width,mOutputVideoSize.height);
        
        [_captureSession commitConfiguration];
        
        _videoOrientation = newOrientation;
    }
    else
    {
        NSLog(@"_videoOrientation == newOrientation");
    }
}

@end
