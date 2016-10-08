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
#import "osxcapture.h"
#include <unistd.h>
#if 0
char *globalFilePathcapture = NULL;
#endif
@implementation ECAVCaptureVideoPreviewLayerEx
- (id)init {
    return [super initWithSession:[[[AVCaptureSession alloc] init] autorelease]];
}
@end

@implementation ECOSXCaptureCCP

@synthesize parentView;
@synthesize triggered;
@synthesize sessionQueue;

#pragma mark - public methods

- (NSNumber *)registerOwner:(cloopenwebrtc::videocapturemodule::VideoCaptureOSX *)owner
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
    
    MSVideoSize size;
    size.height = height;
    size.width = width;
    [self setSize:size];
    [self setFps:frameRate];
    return [NSNumber numberWithInt:0];
}

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
    self.layer = [[ECAVCaptureVideoPreviewLayerEx alloc] init];
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

	NSDictionary* dic = [NSDictionary dictionaryWithObjectsAndKeys:
						 [NSNumber numberWithInteger:kCVPixelFormatType_32ARGB],
                         (id)kCVPixelBufferPixelFormatTypeKey, nil];
	[output setVideoSettings:dic];
    
	start_time=0;
	frame_count=-1;
	fps=0;
    triggered = false;
    
#if 0
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
    static int count = 0;
    NSLog(@"sean count %d", count++);
	CVImageBufferRef frame = nil;
    const int LOCK_FLAGS = 0;
	@synchronized(self) { 
		@try {
			CVImageBufferRef frame = CMSampleBufferGetImageBuffer(sampleBuffer); 
			CVReturn status = CVPixelBufferLockBaseAddress(frame, LOCK_FLAGS);
			if (kCVReturnSuccess != status) {
				frame=nil;
				return;
			}
            void* baseAddress = CVPixelBufferGetBaseAddress(frame);
            size_t bytesPerRow = CVPixelBufferGetBytesPerRow(frame);
            int frameWidth = CVPixelBufferGetWidth(frame);
            int frameHeight = CVPixelBufferGetHeight(frame);
            CVPixelBufferUnlockBaseAddress(frame, LOCK_FLAGS);
            
            NSLog(@"bytesPerRow %zu, frameWidth %d, frameHeight %d", bytesPerRow, frameWidth, frameHeight);
            if(_owner)
            {
        
                int frameSize = bytesPerRow * frameHeight;    // 32 bit ARGB format
                CVBufferRetain(frame);
                VideoCaptureCapability tempCaptureCapability;
                tempCaptureCapability.width = frameWidth;//mOutputVideoSize.width;
                tempCaptureCapability.height = frameHeight;//mOutputVideoSize.height;
                tempCaptureCapability.maxFPS = fps;
                // TODO(wu) : Update actual type and not hard-coded value.
                tempCaptureCapability.rawType = kVideoBGRA;
        
                _owner->IncomingFrame((unsigned char*)baseAddress,
                                      frameSize,
                                      tempCaptureCapability,
                                      0);
        
                CVBufferRelease(frame);
            }
            
		} @finally {
			if (frame) CVPixelBufferUnlockBaseAddress(frame, 0);
            pthread_mutex_unlock(&mutex);
		}
	}
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
    
    NSArray *formatArr = device.formats;
//    NSError *error;
    AVCaptureDeviceFormat * newFormat = formatArr[8];
    //                newFormat.
    if ( [device lockForConfiguration:&error] ) {
        [device setActiveFormat:newFormat];
        //                    [input.device setActiveVideoMinFrameDuration:frameDuration];
        //                    [input.device setActiveVideoMaxFrameDuration:frameDuration];
        [device unlockForConfiguration];
    }
    
    NSLog(@"now active format %@", device.activeFormat);
    
    
    input = [AVCaptureDeviceInput deviceInputWithDevice:device
												  error:&error];
	[input retain]; // keep reference on an externally allocated object
        
	AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
    [session beginConfiguration];
    if (input) {
        if ([session canAddInput:input])
        {
            [session addInput:input];
        }
        else
        {
            NSLog(@"What a pity, session cannot add input");
        }
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
    
	pthread_mutex_destroy(&mutex);
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
        
		[session commitConfiguration];
        BilterFilterInitCore(bilteralFilter, size.width, size.height, 3, 10);
        KeyFrameDetectInitCore(keyframeDector, size.width, size.height);
		return;
	}
}

- (MSVideoSize*)getSize {
	return &mOutputVideoSize;
}

- (void)setFps:(float) value {
//	@synchronized(self) {
//		AVCaptureSession *session = [(AVCaptureVideoPreviewLayer *)self.layer session];
//		[session beginConfiguration];
		{
//            NSArray *formatArr = input.device.formats;
//            NSError *error;
//            AVCaptureDeviceFormat * newFormat = formatArr[8];
//            //                newFormat.
//            if ( [input.device lockForConfiguration:&error] ) {
//                [input.device setActiveFormat:newFormat];
//                //                    [input.device setActiveVideoMinFrameDuration:frameDuration];
//                //                    [input.device setActiveVideoMaxFrameDuration:frameDuration];
//                [input.device unlockForConfiguration];
//            }
//            
//            NSLog(@"now active format %@", input.device.activeFormat);
			
		}
		fps=value;
//		[session commitConfiguration];
//	}
}

- (void)setParentView:(NSView*)aparentView{
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
		{
			previewLayer.videoGravity = AVLayerVideoGravityResize;
		}
        
        [self setFrame: [parentView bounds]];
        [parentView addSubview:self];
	}
}

#if 0
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

@end
//#endif /*TARGET_IPHONE_SIMULATOR*/
