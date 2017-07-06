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


// using openGL draw camera preview.
#define USING_OPENGL;

#ifdef TARGET_OS_IPHONE
#if DEBUG_CAPTURE_YUV
char *globalFilePathcapture = NULL;
#endif
@implementation ECAVCaptureVideoPreviewLayerEx
- (id)init {
    return [super initWithSession:[[[AVCaptureSession alloc] init] autorelease]];
}
@end

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

- (id)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];
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
//	msframe = NULL;
    if (output != NULL) {
        return;
    }
    
	pthread_mutex_init(&mutex, NULL);
	output = [[AVCaptureVideoDataOutput  alloc] init];
	
	[self setOpaque:YES];
	[self setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
	
	/*
	 Currently, the only supported key is kCVPixelBufferPixelFormatTypeKey. Supported pixel formats are kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange and kCVPixelFormatType_32BGRA, except on iPhone 3G, where the supported pixel formats are kCVPixelFormatType_422YpCbCr8 and kCVPixelFormatType_32BGRA..
	 */
	NSDictionary* dic = [NSDictionary dictionaryWithObjectsAndKeys:
						 [NSNumber numberWithInteger:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange],
                         (id)kCVPixelBufferPixelFormatTypeKey, nil];
	[output setVideoSettings:dic];
    
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
#if !TARGET_IPHONE_SIMULATOR
	CVImageBufferRef frame = nil;
	@synchronized(self) { 
		@try {
			frame = CMSampleBufferGetImageBuffer(sampleBuffer); 
			CVReturn status = CVPixelBufferLockBaseAddress(frame, 0);
			if (kCVReturnSuccess != status) {
				frame=nil;
				return;
			}
			
			/*kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange*/
			size_t plane_width = CVPixelBufferGetWidthOfPlane(frame, 0);
//            NSLog(@"sean haha plane_width %zu, plane_height %zu", plane_width, CVPixelBufferGetHeightOfPlane(frame, 0));
            
            
//            size_t plane_widthu = CVPixelBufferGetWidthOfPlane(frame, 1);
     
			size_t plane_height = CVPixelBufferGetHeightOfPlane(frame, 0);
//            size_t plane_heightu = CVPixelBufferGetHeightOfPlane(frame, 1);

            
//            size_t plane_count = CVPixelBufferGetPlaneCount(frame);
            
			uint8_t* y_src= (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(frame, 0);
			uint8_t* cbcr_src= (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(frame, 1);
			int rotation=0;            
            if (![connection isVideoOrientationSupported]) {
				switch (mDeviceOrientation) {
                    case 90: {
						if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
							rotation = 270;
						} else {
							rotation = 90;
						}
						break;
					}
					case 270: {
						if ([(AVCaptureDevice*)input.device position] == AVCaptureDevicePositionBack) {
							rotation = 90;
						} else {
							rotation = 270;
						}
						break;
					}
					default:
                        break;
				}
			}

			/*check if buffer size are compatible with downscaling or rotation*/
			int factor = mDownScalingRequired?2:1;
            int width = mOutputVideoSize.width;
            int height = mOutputVideoSize.height;
            if (mDeviceOrientation ==90 || mDeviceOrientation==270) {
                rotation = 180;
            }
            
			switch (rotation) {
				case 0:
				case 180:
					if (mOutputVideoSize.width*factor>plane_width || mOutputVideoSize.height*factor>plane_height) {
						return;
					}
					break;
				case 90:
				case 270:

					if (mOutputVideoSize.width*factor>plane_height || mOutputVideoSize.height*factor>plane_width) {
						return;
					}
                    width = mOutputVideoSize.height;
                    height = mOutputVideoSize.width;
					break;
                default:
                    break;
			}
            
            
            
//            rotation = 0;
//            if (mDeviceOrientation ==90 || mDeviceOrientation==270) {
//                rotation = 180;
//            }
//            if (rotation==90 || rotation==270) {
//                width = mOutputVideoSize.height;
//                height = mOutputVideoSize.width;
//            }
            MSPicture *pict = copy_ycbcrbiplanar_to_true_yuv_with_rotation_and_down_scale_by_2(
                                            _pict
                                            , y_src
											, cbcr_src
											, rotation
											, width
											, height
											, (int)CVPixelBufferGetBytesPerRowOfPlane(frame, 0)
											, (int)CVPixelBufferGetBytesPerRowOfPlane(frame, 1)
											, TRUE
											, mDownScalingRequired);
            
            pthread_mutex_lock(&mutex);
            if (triggered) {
                BilterFilterProcessCore(bilteralFilter, pict->strides[0], pict->planes[0]);
                int ret = KeyFrameDetectProcess(keyframeDector, pict->strides[0], pict->planes[0], pict->strides[1], pict->planes[1], pict->strides[2], pict->planes[2]);
                if (ret != 1) {
                    return;
                }
            }
            
#if DEBUG_CAPTURE_YUV
            [self saveYUVtoFile:pict->planes[0] andwrap:pict->strides[0] andxsize:width andysize:height];
            [self saveYUVtoFile:pict->planes[1] andwrap:pict->strides[1] andxsize:width/2 andysize:height/2];
            [self saveYUVtoFile:pict->planes[2] andwrap:pict->strides[2] andxsize:width/2 andysize:height/2];
#endif
            I420VideoFrame videoFrame;
//            int size_y = video_frame.height * video_frame.y_pitch;
//            int size_u = video_frame.u_pitch * ((video_frame.height + 1) / 2);
//            int size_v = video_frame.v_pitch * ((video_frame.height + 1) / 2);
            
            int size_y = pict->h * pict->strides[0];
            int size_u = pict->strides[1] * ((pict->h +1) / 2);
            int size_v = pict->strides[2] * ((pict->h + 1)/2);
            
            videoFrame.CreateFrame(size_y, pict->planes[0], size_u, pict->planes[1], size_v, pict->planes[2], pict->w, pict->h, pict->strides[0], pict->strides[1], pict->strides[2]);
#ifdef USING_OPENGL
            if (_renderCallback) {
               // _renderCallback->RenderFrame(0, videoFrame);
            }
#endif
            if(_owner)
            {
                _owner->IncomingI420VideoFrame(&videoFrame, 0);
            }
            
		} @finally {
			if (frame) CVPixelBufferUnlockBaseAddress(frame, 0);
            pthread_mutex_unlock(&mutex);
		}
	}
#endif
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
        device.subjectAreaChangeMonitoringEnabled=YES;
        [device unlockForConfiguration];
    }else{
        NSLog(@"enable area change monitor error：%@",error.localizedDescription);
    }

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(subjectAreaDidChange:) name:AVCaptureDeviceSubjectAreaDidChangeNotification object:device];
    
	input = [AVCaptureDeviceInput deviceInputWithDevice:device
												  error:&error];
	[input retain]; // keep reference on an externally allocated object
        
	AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
    [session beginConfiguration];
    if (input) {
        [session addInput:input];
    }
	[session addOutput:output];
    [session commitConfiguration];
    
    
}

- (void)dealloc {
	AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
	[session removeInput:input];
	[session removeOutput:output];
	[output release];
    [input release];
    
    BilterFilterFree(bilteralFilter);
    KeyFrameDetectFree(keyframeDector);
    
    if(self.parentView)
    {
        [self removeFromSuperview];
        [parentView release];
        parentView = nil;
    }
#ifdef USING_OPENGL
    if (_ptrRenderer) {
        _ptrRenderer->StopRender();
        delete _ptrRenderer;
        _ptrRenderer = NULL;
    }
#endif
    
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

+ (Class)layerClass {   
	return [ECAVCaptureVideoPreviewLayerEx class];
}

- (int)start {
	NSAutoreleasePool* myPool = [[NSAutoreleasePool alloc] init];
	@synchronized(self) {
        CGPoint devicePoint = CGPointMake( 0.5, 0.5 );
        [self focusWithMode:AVCaptureFocusModeContinuousAutoFocus exposeWithMode:AVCaptureExposureModeContinuousAutoExposure atDevicePoint:devicePoint monitorSubjectAreaChange:NO];
		AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
		if (!session.running) {
			// Init queue
			dispatch_queue_t queue = dispatch_queue_create("CaptureQueue", NULL);
			dispatch_set_context(queue, [self retain]);
			dispatch_set_finalizer_f(queue, capture_queue_cleanup);
			[output setSampleBufferDelegate:self queue:queue];
            //output.alwaysDiscardsLateVideoFrames = false;
			dispatch_release(queue);
			[session startRunning]; //warning can take around 1s before returning
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
		AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
        [session beginConfiguration];
        for (AVCaptureInput *oldInput in [session inputs]) {
            [session removeInput:oldInput];
        }
        [session commitConfiguration];
		if (session.running) {
			[session stopRunning];
			
			// Will free the queue
			[output setSampleBufferDelegate:nil queue:nil];
		}
	}
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];
//    [[NSNotificationCenter defaultCenter] removeObserver:self name:AVCaptureDeviceSubjectAreaDidChangeNotification object:nil];
    
    [self removeFromSuperview];
        
	[myPool drain];    
	return 0;
}

- (void)setSize:(MSVideoSize) size {
	@synchronized(self) {
		AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
		[session beginConfiguration];
		if (size.width*size.height == MS_VIDEO_SIZE_QVGA_W  * MS_VIDEO_SIZE_QVGA_H)
        {
			[session setSessionPreset: AVCaptureSessionPreset640x480];
			mCameraVideoSize.width=MS_VIDEO_SIZE_VGA_W;
			mCameraVideoSize.height=MS_VIDEO_SIZE_VGA_H;
			mOutputVideoSize.width=MS_VIDEO_SIZE_QVGA_W;
			mOutputVideoSize.height=MS_VIDEO_SIZE_QVGA_H;
			mDownScalingRequired=true;
		}
        else if (size.width*size.height == MS_VIDEO_SIZE_720P_W * MS_VIDEO_SIZE_720P_H)
        {
            [session setSessionPreset: AVCaptureSessionPreset1280x720];
            mCameraVideoSize.width=MS_VIDEO_SIZE_720P_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_720P_H;
            mOutputVideoSize.width=MS_VIDEO_SIZE_720P_W;
            mOutputVideoSize.height=MS_VIDEO_SIZE_720P_H;
            mDownScalingRequired=false;
        }
        else if (size.width*size.height == MS_VIDEO_SIZE_960_540_W * MS_VIDEO_SIZE_960_540_H)
        {
            [session setSessionPreset: AVCaptureSessionPresetiFrame960x540];
            mCameraVideoSize.width=MS_VIDEO_SIZE_960_540_W;
            mCameraVideoSize.height=MS_VIDEO_SIZE_960_540_H;
            mOutputVideoSize.width=MS_VIDEO_SIZE_960_540_W;
            mOutputVideoSize.height=MS_VIDEO_SIZE_960_540_H;
            mDownScalingRequired=false;
        }
        else if (size.width*size.height == MS_VIDEO_SIZE_VGA_W  * MS_VIDEO_SIZE_VGA_H) {
			[session setSessionPreset: AVCaptureSessionPreset640x480];
			mCameraVideoSize.width=MS_VIDEO_SIZE_VGA_W;
			mCameraVideoSize.height=MS_VIDEO_SIZE_VGA_H;
			mOutputVideoSize=mCameraVideoSize;
			mDownScalingRequired=false;
		} else {
			[session setSessionPreset: AVCaptureSessionPresetMedium];
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
			mOutputVideoSize.width=tmpSize.height;
			mOutputVideoSize.height=tmpSize.width;
		}
        [self changeSize];
        
		[session commitConfiguration];
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
		AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
		[session beginConfiguration];
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
		fps=value;
		snprintf(fps_context, sizeof(fps_context), "Captured mean fps=%%f, expected=%f", fps);
		ms_video_init_average_fps(&averageFps, fps_context);
		[session commitConfiguration];
	}
}

- (void)setParentView:(UIView*)aparentView{
#ifndef USING_OPENGL
    if (parentView == aparentView) {
		return;
	}    

	if(parentView != nil) {
		[self removeFromSuperview];
		[parentView release];
		parentView = nil;
	}
	parentView = aparentView;
	
	if(parentView != nil) {
        [parentView retain];
		AVCaptureVideoPreviewLayer *previewLayer = (AVCaptureVideoPreviewLayer *)self.layer;
		if([parentView contentMode] == UIViewContentModeScaleAspectFit) {
			previewLayer.videoGravity = AVLayerVideoGravityResizeAspect;
		} else if([parentView contentMode] == UIViewContentModeScaleAspectFill) {
			previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
		} else {
			previewLayer.videoGravity = AVLayerVideoGravityResize;
		}
		 [parentView insertSubview:self atIndex:0];
		 [self setFrame: [parentView bounds]];
	}
#else
    // create render and add renderFrame view
//    VideoRenderIPhoneImpl* ptrRenderer = new VideoRenderIPhoneImpl(0, kRenderiOS, (void*)aparentView, false);
//    if (ptrRenderer) {
//       _ptrRenderer =  reinterpret_cast<cloopenwebrtc::IVideoRender*>(ptrRenderer);
//    }
//    
//    if (_ptrRenderer->Init() == -1) {
//        NSLog( @"Initialize VideoRenderIPhoneImpl failure\n");
//    }
//    
//    // @see: http://blog.csdn.net/cjj198561/article/details/34196305
//     _renderCallback = ptrRenderer->AddIncomingRenderStream(0, 0, 0.0, 0.0, 1.0, 1.0);
//     _ptrRenderer->StartRender();
#endif
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
