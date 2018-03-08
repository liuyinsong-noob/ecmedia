/*
 ioscapture.m
 Copyright (C) 2011 Belledonne Communications, Grenoble, France
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#import "ioscapture.h"
#import <pthread.h>

#include "video_render_iphone_impl.h"
#include "video_render_defines.h"
#include "i_video_render.h"
#include "video_render_impl.h"

#ifdef TARGET_OS_IPHONE
#if DEBUG_CAPTURE_YUV
char *globalFilePathcapture = NULL;
#endif


@implementation ECIOSCaptureCCP

@synthesize parentView;
@synthesize triggered;
@synthesize sessionQueue;

#pragma mark - public methods

- (NSNumber *)registerOwner:(cloopenwebrtc::videocapturemodule::VideoCaptureiOS *)owner
{
//    if (!owner) {
//        return [NSNumber numberWithInt:-1];
//    }
    _owner = owner;
    return [NSNumber numberWithInt:0];
}


- (NSNumber*)setCaptureDeviceById:(char*)uniqueId
{
    [self openDevice:uniqueId];
    return 0;
}


- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate
{
    BOOL isauto = false;
    if ([self respondsToSelector:@selector(isAutoOrientation)]) {
        if ([self isAutoOrientation]) {
            isauto = YES;
        }
    }
    
    if (isauto) {
        UIDeviceOrientation oritentation = [[UIDevice currentDevice] orientation];
        
        if (oritentation == UIInterfaceOrientationPortrait ) {
            mDeviceOrientation = 0;
        } else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
            mDeviceOrientation = 270;
        } else if (oritentation == UIInterfaceOrientationLandscapeLeft ) {
            mDeviceOrientation = 90;
        }else if(oritentation == UIInterfaceOrientationMaskAllButUpsideDown) {
            mDeviceOrientation = 180;
        }
    } else {
        switch (mRotate) {
            case kCameraRotate0:
                mDeviceOrientation = 0;
                break;
            case kCameraRotate90:
                if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
                    mDeviceOrientation = 90;
                } else {
                    mDeviceOrientation = 270;
                }
                break;
            case kCameraRotate180:
                mDeviceOrientation = 180;
                break;
            case kCameraRotate270:
                if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
                    mDeviceOrientation = 270;
                } else {
                    mDeviceOrientation = 90;
                }
                break;
        }
    }
    /*
    UIDeviceOrientation oritentation = [[UIDevice currentDevice] orientation];
    
    if (oritentation == UIInterfaceOrientationPortrait ) {
		mDeviceOrientation = 0;		
	} else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
        mDeviceOrientation = 270;
	} else if (oritentation == UIInterfaceOrientationLandscapeLeft ) {
        mDeviceOrientation = 90;
	}else if(oritentation == UIInterfaceOrientationMaskAllButUpsideDown) {
        mDeviceOrientation = 180;
    }
    */
    
    MSVideoSize size;
    size.height = height;
    size.width = width;
    [self setSize:size];
    [self setFps:frameRate];
    return [NSNumber numberWithInt:0];
}

#ifdef __APPLE_CC__
- (void)setCaptureRotate:(VideoCaptureRotation)rotate {
    BOOL isauto = false;
    if ([self respondsToSelector:@selector(isAutoOrientation)]) {
        if ([self isAutoOrientation]) {
            isauto = YES;
        }
    }
    if (isauto) {
        UIDeviceOrientation oritentation = [[UIDevice currentDevice] orientation];
        
        if (oritentation == UIInterfaceOrientationPortrait ) {
            mDeviceOrientation = 0;
        } else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
            mDeviceOrientation = 270;
        } else if (oritentation == UIInterfaceOrientationLandscapeLeft ) {
            mDeviceOrientation = 90;
        }else if(oritentation == UIInterfaceOrientationMaskAllButUpsideDown) {
            mDeviceOrientation = 180;
        }
    } else {
        mRotate = rotate;
        switch (mRotate) {
            case kCameraRotate0:
                mDeviceOrientation = 0;
                break;
            case kCameraRotate90:
            {
                if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
                    mDeviceOrientation = 90;
                } else {
                    mDeviceOrientation = 270;
                }
                
            }
                break;
            case kCameraRotate180:
                mDeviceOrientation = 180;
                break;
            case kCameraRotate270:
            {
                if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
                    mDeviceOrientation = 270;
                } else {
                    mDeviceOrientation = 90;
                }
            }
                break;
        }
    }
    [self setSize:mOutputVideoSize];
}
#endif

- (NSNumber*)startCapture
{    
    return [NSNumber numberWithInt:[self start]];
}
- (NSNumber*)stopCapture
{    
    return [NSNumber numberWithInt:[self stop]];
}

- (id)init {
	self = [super init];
	if (self) {
		[self initIOSCapture];
	}

	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initIOSCapture];
	}
	return self;
}

- (void)initIOSCapture {
    if(_capture_session == nullptr) {
        _capture_session = [[AVCaptureSession alloc] init];
    }
    
//	msframe = NULL;
    if (output != NULL) {
        return;
    }
    
	pthread_mutex_init(&mutex, NULL);
	output = [[AVCaptureVideoDataOutput  alloc] init];
	[output setAlwaysDiscardsLateVideoFrames:NO];
    
	/*
	 Currently, the only supported key is kCVPixelBufferPixelFormatTypeKey. Supported pixel formats are kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange and kCVPixelFormatType_32BGRA, except on iPhone 3G, where the supported pixel formats are kCVPixelFormatType_422YpCbCr8 and kCVPixelFormatType_32BGRA..
	 */
	[output setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
    
	start_time=0;
	frame_count=-1;
	fps=0;
    triggered = false;
    
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver: self selector:   @selector(deviceOrientationNotify) name: UIDeviceOrientationDidChangeNotification object: nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(subjectAreaDidChange:) name:AVCaptureDeviceSubjectAreaDidChangeNotification object:input.device];
    
    _pict = (MSPicture *)malloc(sizeof(MSPicture));
    _pict->ptr = NULL;
#if DEBUG_CAPTURE_YUV
    if (globalFilePathcapture) {
        fout = fopen(globalFilePathcapture, "ab+");
    }
#endif
    
    self.sessionQueue = dispatch_queue_create( "session queue", DISPATCH_QUEUE_SERIAL );
    
    CreateBilterFilter(&bilteralFilter);
    CreateKeyFrameDetect(&keyframeDector);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
	   fromConnection:(AVCaptureConnection *)connection {
    
    
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(imageBuffer,0);
    int bytesPerRow = (int) CVPixelBufferGetBytesPerRow(imageBuffer);
    // argb image width
    int width = (int) CVPixelBufferGetWidth(imageBuffer);
    // argb image height
    int  height = (int)CVPixelBufferGetHeight(imageBuffer);
    GLubyte *src_buff = (GLubyte*)CVPixelBufferGetBaseAddress(imageBuffer);
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
    int extraBytes = bytesPerRow - width*4;

    
    GLubyte *image_buffer = (GLubyte*)malloc(width*height*4);
    for(int i= 0; i< height; i++) {
        memcpy(image_buffer+i*width*4, src_buff+bytesPerRow*i, width*4);
    }
    
    
    if(_rawDataInput) {
        [_rawDataInput processARGBData:image_buffer imageSize:CGSizeMake(width, height)];
    }
    free(image_buffer);
    return;
//
//#if !TARGET_IPHONE_SIMULATOR
//    CVImageBufferRef frame = nil;
//    @synchronized(self) {
//        @try {
//            frame = CMSampleBufferGetImageBuffer(sampleBuffer);
//            CVReturn status = CVPixelBufferLockBaseAddress(frame, 0);
//            if (kCVReturnSuccess != status) {
//                frame=nil;
//                return;
//            }
//
//            /*kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange*/
//            size_t plane_width = CVPixelBufferGetWidthOfPlane(frame, 0);
////            NSLog(@"sean haha plane_width %zu, plane_height %zu", plane_width, CVPixelBufferGetHeightOfPlane(frame, 0));
//
//
////            size_t plane_widthu = CVPixelBufferGetWidthOfPlane(frame, 1);
//
//            size_t plane_height = CVPixelBufferGetHeightOfPlane(frame, 0);
////            size_t plane_heightu = CVPixelBufferGetHeightOfPlane(frame, 1);
//
//
////            size_t plane_count = CVPixelBufferGetPlaneCount(frame);
//
//            uint8_t* y_src= (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(frame, 0);
//            uint8_t* cbcr_src= (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(frame, 1);
//            int rotation=0;
//            if (![connection isVideoOrientationSupported]) {
//                switch (mDeviceOrientation) {
//                    case 90: {
//                        if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
//                            rotation = 270;
//                        } else {
//                            rotation = 90;
//                        }
//                        break;
//                    }
//                    case 270: {
//                        if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
//                            rotation = 90;
//                        } else {
//                            rotation = 270;
//                        }
//                        break;
//                    }
//                    default:
//                        break;
//                }
//            }
//
//            /*check if buffer size are compatible with downscaling or rotation*/
//            int factor = mDownScalingRequired?2:1;
//            int width = mOutputVideoSize.width;
//            int height = mOutputVideoSize.height;
//            if (mDeviceOrientation ==90 || mDeviceOrientation==270) {
//                rotation = 180;
//            }
//
//            switch (rotation) {
//                case 0:
//                case 180:
//                    if (mOutputVideoSize.width*factor>plane_width || mOutputVideoSize.height*factor>plane_height) {
//                        return;
//                    }
//                    break;
//                case 90:
//                case 270:
//
//                    if (mOutputVideoSize.width*factor>plane_height || mOutputVideoSize.height*factor>plane_width) {
//                        return;
//                    }
//                    width = mOutputVideoSize.height;
//                    height = mOutputVideoSize.width;
//                    break;
//                default:
//                    break;
//            }
//
//
//
////            rotation = 0;
////            if (mDeviceOrientation ==90 || mDeviceOrientation==270) {
////                rotation = 180;
////            }
////            if (rotation==90 || rotation==270) {
////                width = mOutputVideoSize.height;
////                height = mOutputVideoSize.width;
////            }
//            MSPicture *pict = copy_ycbcrbiplanar_to_true_yuv_with_rotation_and_down_scale_by_2(
//                                            _pict
//                                            , y_src
//                                            , cbcr_src
//                                            , rotation
//                                            , width
//                                            , height
//                                            , (int)CVPixelBufferGetBytesPerRowOfPlane(frame, 0)
//                                            , (int)CVPixelBufferGetBytesPerRowOfPlane(frame, 1)
//                                            , TRUE
//                                            , mDownScalingRequired);
//
//            pthread_mutex_lock(&mutex);
//            if (triggered) {
//                BilterFilterProcessCore(bilteralFilter, pict->strides[0], pict->planes[0]);
//                int ret = KeyFrameDetectProcess(keyframeDector, pict->strides[0], pict->planes[0], pict->strides[1], pict->planes[1], pict->strides[2], pict->planes[2]);
//                if (ret != 1) {
//                    return;
//                }
//            }
//
//#if DEBUG_CAPTURE_YUV
//            [self saveYUVtoFile:pict->planes[0] andwrap:pict->strides[0] andxsize:width andysize:height];
//            [self saveYUVtoFile:pict->planes[1] andwrap:pict->strides[1] andxsize:width/2 andysize:height/2];
//            [self saveYUVtoFile:pict->planes[2] andwrap:pict->strides[2] andxsize:width/2 andysize:height/2];
//#endif
//            I420VideoFrame videoFrame;
////            int size_y = video_frame.height * video_frame.y_pitch;
////            int size_u = video_frame.u_pitch * ((video_frame.height + 1) / 2);
////            int size_v = video_frame.v_pitch * ((video_frame.height + 1) / 2);
//
//            int size_y = pict->h * pict->strides[0];
//            int size_u = pict->strides[1] * ((pict->h +1) / 2);
//            int size_v = pict->strides[2] * ((pict->h + 1)/2);
//
//            videoFrame.CreateFrame(size_y, pict->planes[0], size_u, pict->planes[1], size_v, pict->planes[2], pict->w, pict->h, pict->strides[0], pict->strides[1], pict->strides[2]);
//            if(_owner)
//            {
//                _owner->IncomingI420VideoFrame(&videoFrame, 0);
//            }
//
//        } @finally {
//            if (frame) CVPixelBufferUnlockBaseAddress(frame, 0);
//            pthread_mutex_unlock(&mutex);
//        }
//    }
//#endif
}

- (void)openDevice:(const char*) deviceId {
	NSError *error = nil;
	unsigned int i = 0;
	AVCaptureDevice * device = NULL;
    	
	NSArray * array = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
	for (i = 0 ; i < [array count]; i++) {
		AVCaptureDevice * currentDevice = [array objectAtIndex:i];
		if(!strcmp([[currentDevice uniqueID] UTF8String], deviceId)) {
			device = currentDevice;
			break;
		}
	}
	if (device == NULL) {
		device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	}
    if ([device lockForConfiguration:&error]) {
        device.subjectAreaChangeMonitoringEnabled = YES;
        [device unlockForConfiguration];
    }else{
        NSLog(@"enable area change monitor error：%@",error.localizedDescription);
    }

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(subjectAreaDidChange:) name:AVCaptureDeviceSubjectAreaDidChangeNotification object:device];
    
	input = [AVCaptureDeviceInput deviceInputWithDevice:device
												  error:&error];
	[input retain]; // keep reference on an externally allocated object
    
    [_capture_session beginConfiguration];
    if (input) {
        [_capture_session addInput:input];
    }
	[_capture_session addOutput:output];
    [_capture_session commitConfiguration];
}

- (void)dealloc {
	[_capture_session removeInput:input];
	[_capture_session removeOutput:output];
	[output release];
    [input release];
    
    BilterFilterFree(bilteralFilter);
    KeyFrameDetectFree(keyframeDector);
    
    if(self.parentView)
    {
        [parentView release];
        parentView = nil;
    }
    
    if(_pict)
    {
        if(_pict->ptr)
            free(_pict->ptr);
        free(_pict);
    }
	pthread_mutex_destroy(&mutex);
    
#ifdef DEBUG_CAPTURE_YUV
    if (fout)
    {
        fflush(fout);
        fclose(fout);
    }
#endif
#if ! __has_feature(objc_arc)
	[super dealloc];
#endif
}

- (int)start {
	NSAutoreleasePool* myPool = [[NSAutoreleasePool alloc] init];
	@synchronized(self) {
        CGPoint devicePoint = CGPointMake( 0.5, 0.5 );
        [self focusWithMode:AVCaptureFocusModeContinuousAutoFocus exposeWithMode:AVCaptureExposureModeContinuousAutoExposure atDevicePoint:devicePoint monitorSubjectAreaChange:NO];
		 
		if (!_capture_session.running) {
			// Init queue
			dispatch_queue_t queue = dispatch_queue_create("CaptureQueue", NULL);
			dispatch_set_context(queue, [self retain]);
			dispatch_set_finalizer_f(queue, capture_queue_cleanup);
			[output setSampleBufferDelegate:self queue:queue];
            //output.alwaysDiscardsLateVideoFrames = false;
			dispatch_release(queue);
			[_capture_session startRunning]; //warning can take around 1s before returning
			snprintf(fps_context, sizeof(fps_context), "Captured mean fps=%%f, expected=%f", fps);
			ms_video_init_average_fps(&averageFps, fps_context);
			//NSLog(@"ioscapture video device started.");
		}
	}
	[myPool drain];
	return 0;
}

- (int)stop {
    
	NSAutoreleasePool* myPool = [[NSAutoreleasePool alloc] init];
	@synchronized(self) {
        [_capture_session beginConfiguration];
        for (AVCaptureInput *oldInput in [_capture_session inputs]) {
            [_capture_session removeInput:oldInput];
        }
        [_capture_session commitConfiguration];
		if (_capture_session.running) {
			[_capture_session stopRunning];
			
			// Will free the queue
			[output setSampleBufferDelegate:nil queue:nil];
		}
	}
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];
//    [[NSNotificationCenter defaultCenter] removeObserver:self name:AVCaptureDeviceSubjectAreaDidChangeNotification object:nil];
 
    if(_ecImageView.superview) {
        [_ecImageView removeFromSuperview];
        _ecImageView  = nullptr;
    }
    
    if(_ecImageFilter) {
        [_ecImageFilter removeAllTargets];
    }

	[myPool drain];    
	return 0;
}

- (void)setSize:(MSVideoSize) size {
	@synchronized(self) {

        if (size.width*size.height == MS_VIDEO_SIZE_QVGA_W  * MS_VIDEO_SIZE_QVGA_H)
        {
            [_capture_session setSessionPreset: AVCaptureSessionPreset640x480];
            mCameraVideoSize.width=MS_VIDEO_SIZE_VGA_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_VGA_H;
            mOutputVideoSize.width=MS_VIDEO_SIZE_QVGA_W;
            mOutputVideoSize.height=MS_VIDEO_SIZE_QVGA_H;
            mDownScalingRequired=true;
        }
        else if (size.width*size.height == MS_VIDEO_SIZE_720P_W * MS_VIDEO_SIZE_720P_H)
        {
            [_capture_session setSessionPreset: AVCaptureSessionPreset1280x720];
            mCameraVideoSize.width=MS_VIDEO_SIZE_720P_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_720P_H;
            mOutputVideoSize.width=MS_VIDEO_SIZE_720P_W;
            mOutputVideoSize.height=MS_VIDEO_SIZE_720P_H;
            mDownScalingRequired=false;
        }
        else if (size.width*size.height == MS_VIDEO_SIZE_960_540_W * MS_VIDEO_SIZE_960_540_H)
        {
            [_capture_session setSessionPreset: AVCaptureSessionPresetiFrame960x540];
            mCameraVideoSize.width=MS_VIDEO_SIZE_960_540_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_960_540_H;
            mOutputVideoSize.width=MS_VIDEO_SIZE_960_540_W;
            mOutputVideoSize.height=MS_VIDEO_SIZE_960_540_H;
            mDownScalingRequired=false;
        }
        else if (size.width*size.height == MS_VIDEO_SIZE_VGA_W  * MS_VIDEO_SIZE_VGA_H) {
            [_capture_session setSessionPreset: AVCaptureSessionPreset640x480];
            mCameraVideoSize.width=MS_VIDEO_SIZE_VGA_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_VGA_H;
            mOutputVideoSize=mCameraVideoSize;
            mDownScalingRequired=false;
        } else {
            [_capture_session setSessionPreset: AVCaptureSessionPresetMedium];
            mCameraVideoSize.width=MS_VIDEO_SIZE_IOS_MEDIUM_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_IOS_MEDIUM_H;
            mOutputVideoSize=mCameraVideoSize;
            mDownScalingRequired=false;
        }
		
		NSArray *connections = output.connections;
		if ([connections count] > 0 && [[connections objectAtIndex:0] isVideoOrientationSupported]) {
			switch (mDeviceOrientation) {
                case 0:
					[[connections objectAtIndex:0] setVideoOrientation:AVCaptureVideoOrientationPortrait];
					break;
				case 180:
					[[connections objectAtIndex:0] setVideoOrientation:AVCaptureVideoOrientationPortraitUpsideDown];
					break;
				case 270://90:
					[[connections objectAtIndex:0] setVideoOrientation:AVCaptureVideoOrientationLandscapeRight];
					break;
				case 90://270:
					[[connections objectAtIndex:0] setVideoOrientation:AVCaptureVideoOrientationLandscapeLeft];
				default:
					break;
			}
		}
        
		if (mDeviceOrientation == 0 || mDeviceOrientation == 180) {
			MSVideoSize tmpSize = mOutputVideoSize;
			mOutputVideoSize.width = tmpSize.height;
			mOutputVideoSize.height = tmpSize.width;
		}
        [self changeSize];
        
		[_capture_session commitConfiguration];
        BilterFilterInitCore(bilteralFilter, size.width, size.height, 3, 10);
        KeyFrameDetectInitCore(keyframeDector, size.width, size.height);
		return;
	}
}

- (void)changeSize
{
    pthread_mutex_lock(&mutex);
    if(_pict && _pict->ptr)
    {
        free(_pict->ptr);
    }
    int w = mOutputVideoSize.width;
    int h = mOutputVideoSize.height;
    
    _pict->ptr = (uint8_t *)malloc((w*h*3)/2);
    _pict->w=w;
	_pict->h=h;
    _pict->planes[0] = (uint8_t *)_pict->ptr;
    _pict->planes[1] = (uint8_t *)(_pict->planes[0] + w*h);
    _pict->planes[2] = (uint8_t *)(_pict->planes[1] + w*h/4);
    _pict->planes[3] = 0;
    pthread_mutex_unlock(&mutex);
}

- (MSVideoSize*)getSize {
	return &mOutputVideoSize;
}

- (void)setFps:(float) value {
	@synchronized(self) {
		[_capture_session beginConfiguration];
		if ([[[UIDevice currentDevice] systemVersion] floatValue] < 5) { 
			[output setMinFrameDuration:CMTimeMake(1, value)];
		} else {
			NSArray *connections = output.connections;
			if ([connections count] > 0) {
                CMTime frameDuration = CMTimeMake(1, value);
                [input.device setActiveVideoMinFrameDuration:frameDuration];
                [input.device setActiveVideoMaxFrameDuration:frameDuration];
			}
			
		}
		fps = value;
		snprintf(fps_context, sizeof(fps_context), "Captured mean fps=%%f, expected=%f", fps);
		ms_video_init_average_fps(&averageFps, fps_context);
		[_capture_session commitConfiguration];
	}
}

- (void)setParentView:(UIView*)aparentView{
    if (parentView == aparentView) {
        return;
    }
    
    if(parentView != nil && _ecImageView != nil) {
        [_ecImageView removeFromSuperview];
        [parentView release];
        parentView = nil;
    }
    parentView = aparentView;
    if(parentView != nil) {
        [parentView retain];
        if(_ecImageView == nil) {
            _ecImageView = [[ECImageView alloc] initWithFrame:CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height)];
            if([parentView contentMode] == UIViewContentModeScaleAspectFit) {
                [_ecImageView setFillMode:kECImageFillModePreserveAspectRatio];
            } else if([parentView contentMode] == UIViewContentModeScaleAspectFill) {
                [_ecImageView setFillMode:kECImageFillModePreserveAspectRatioAndFill];
            } else {
                [_ecImageView setFillMode:kECImageFillModeStretch];
            }
            // view horizontal mirror. zhaoyou
            if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionFront) {
                [_ecImageView setInputRotation:kECImageFlipHorizonal atIndex:0];
            }
            [_ecImageView setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
        }
        [_ecImageView setFrame:CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height)];
        [parentView insertSubview:_ecImageView atIndex:0];
        
        if(_rawDataInput == nullptr) {
            _rawDataInput = [[ECImageRawDataInput alloc] initWithBytes:(GLubyte *)nullptr size:CGSizeMake(0, 0)];
            _rawDataOutput = [[ECImageRawDataOutput alloc] initWithImageSize:CGSizeMake(mOutputVideoSize.width, mOutputVideoSize.height) resultsInBGRAFormat:YES];
            [_rawDataOutput setI420FrameAvailableBlock:^(const GLubyte *outputBytes, uint8_t *bytes_y, int stride_y, uint8_t *bytes_u, int stride_u, uint8_t *bytes_v, int stride_v, NSInteger width, int height) {
                pthread_mutex_lock(&mutex);
                I420VideoFrame videoFrame;
                videoFrame.CreateFrame(width*height, bytes_y, width*height/4, bytes_u, width*height/4, bytes_v, width, height, stride_y, stride_u, stride_v);
                
                if(_owner)  {
                    _owner->IncomingI420VideoFrame(&videoFrame, 0);
                }
                pthread_mutex_unlock(&mutex);
            }];
            
            [_rawDataInput addTarget:_rawDataOutput];
            [_rawDataInput addTarget:_ecImageView];
        }
        
    }
}


-(void)setBeautyFace:(BOOL)isEnable {
    if(isEnable) {
        [_rawDataInput removeAllTargets];
        if(_ecImageFilter != nullptr) {
           [_ecImageFilter removeOutputFramebuffer];
        }
         _ecImageFilter = [ECImageFilterFactory createImageFiilterWithType:cloopenwebrtc::ECType_BeautyFaceFilter];
        // view horizontal mirror. zhaoyou
        [_ecImageView setInputRotation:kECImageFlipHorizonal atIndex:0];
        [_rawDataInput addTarget:_ecImageFilter];
        [_ecImageFilter addTarget:_rawDataOutput];
        [_ecImageFilter addTarget:_ecImageView];
    } else {
        [_rawDataInput removeAllTargets];
        [_rawDataInput addTarget:_rawDataOutput];
        [_rawDataInput addTarget:_ecImageView];
    }
}

- (void)setVideoFilter:(ECImageFilterType) filter {
    [_rawDataInput removeAllTargets];
    if(_ecImageFilter != nullptr) {
        [_ecImageFilter removeOutputFramebuffer];
    }
    _ecImageFilter = [ECImageFilterFactory createImageFiilterWithType:filter];
    // view horizontal mirror. zhaoyou
    [_ecImageView setInputRotation:kECImageFlipHorizonal atIndex:0];
    [_rawDataInput addTarget:_ecImageFilter];
    [_ecImageFilter addTarget:_rawDataOutput];
    [_ecImageFilter addTarget:_ecImageView];
}

- (void)deviceOrientationNotify {
#ifdef __APPLE_CC__    
    BOOL isauto = false;
    if ([self respondsToSelector:@selector(isAutoOrientation)]) {
        if ([self isAutoOrientation]) {
            isauto = YES;
        }
    }
    if (!isauto) {
        return;
    }
#endif

    int deviceOrientation=0;
    UIDeviceOrientation oritentation = [[UIDevice currentDevice] orientation];//
//    UIDeviceOrientation oritentation = UIInterfaceOrientationPortrait;//[[UIDevice currentDevice] orientation];//
    if (oritentation == UIInterfaceOrientationPortrait ) {
		deviceOrientation = 0;
	} else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
        deviceOrientation = 270;
	} else if (oritentation == UIInterfaceOrientationLandscapeLeft ) {
        deviceOrientation = 90;
	}else if(oritentation == UIDeviceOrientationPortraitUpsideDown) {
        deviceOrientation = 180;
    }
        
    if (mDeviceOrientation != deviceOrientation) {
        mDeviceOrientation = deviceOrientation;
        [self setSize:mOutputVideoSize]; //to update size from orientation
    }
}

#if DEBUG_CAPTURE_YUV
- (void)saveYUVtoFile:(unsigned char *)buf andwrap:(int)wrap andxsize:(int)xsize andysize:(int)ysize
{
    if (!fout) {
        return;
    }
    int i;
    flockfile(fout);
    for(i=0;i<ysize;i++)
    {
        fwrite(buf + i * wrap, 1, xsize, fout);
    }
    funlockfile(fout);
}
#endif

- (NSNumber *)updateLossRate:(int)lossRate
{
    //    NSLog(@"sean haha lossRate %d",lossRate);
    if (lossRate > 30) {
        triggered = true;
    }
    else
        triggered = false;
    return [NSNumber numberWithInt:0];
}

- (void)subjectAreaDidChange:(NSNotification *)notification
{
    CGPoint devicePoint = CGPointMake( 0.5, 0.5 );
    [self focusWithMode:AVCaptureFocusModeContinuousAutoFocus exposeWithMode:AVCaptureExposureModeContinuousAutoExposure atDevicePoint:devicePoint monitorSubjectAreaChange:NO];
}

- (void)focusWithMode:(AVCaptureFocusMode)focusMode exposeWithMode:(AVCaptureExposureMode)exposureMode atDevicePoint:(CGPoint)point monitorSubjectAreaChange:(BOOL)monitorSubjectAreaChange
{
    dispatch_async( self.sessionQueue, ^{
        AVCaptureDevice *device = input.device;
        NSError *error = nil;
        if ( [device lockForConfiguration:&error] ) {
            // Setting (focus/exposure)PointOfInterest alone does not initiate a (focus/exposure) operation.
            // Call -set(Focus/Exposure)Mode: to apply the new point of interest.
            if ( device.isFocusPointOfInterestSupported && [device isFocusModeSupported:focusMode] ) {
                device.focusPointOfInterest = point;
                device.focusMode = focusMode;
            }
            
            if ( device.isExposurePointOfInterestSupported && [device isExposureModeSupported:exposureMode] ) {
                device.exposurePointOfInterest = point;
                device.exposureMode = exposureMode;
            }
            
            device.subjectAreaChangeMonitoringEnabled = monitorSubjectAreaChange;
            [device unlockForConfiguration];
        }
        else {
            NSLog( @"Could not lock device for configuration: %@", error );
        }
    } );
}
@end
//#endif /*TARGET_IPHONE_SIMULATOR*/
#endif
