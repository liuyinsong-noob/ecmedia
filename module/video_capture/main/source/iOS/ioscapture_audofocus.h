
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#ifdef TARGET_OS_IPHONE
#import <QuartzCore/CALayer.h>
#import "video_capture_defines.h"
#import "video_capture_iOS_ObjC.h"
#import <stdint.h>

using namespace cloopenwebrtc;
using namespace videocapturemodule;

#if !TARGET_IPHONE_SIMULATOR
#include "msvideo.h"
#endif

#include "bilteral_filter.h"
#include "keyframe_detector.h"

// AVCaptureVideoPreviewLayer with AVCaptureSession creation
@interface ECAVCaptureVideoPreviewLayerEx : AVCaptureVideoPreviewLayer
@end

@interface ECIOSCaptureCCP : UIView<AVCaptureVideoDataOutputSampleBufferDelegate> {
@private
    AVCaptureDeviceInput *input;
    AVCaptureVideoDataOutput * output;
    pthread_mutex_t mutex;
    int frame_ind;
    float fps;
    float start_time;
    int frame_count;
    MSVideoSize mOutputVideoSize;
    MSVideoSize mCameraVideoSize; //required size in portrait mode
    Boolean mDownScalingRequired;
    int mDeviceOrientation;
    #ifdef __APPLE_CC__
    VideoCaptureRotation mRotate;
    #endif
    MSAverageFPS averageFps;
    char fps_context[64];
    MSPicture *_pict;
    cloopenwebrtc::videocapturemodule::VideoCaptureiOS* _owner;
    //    UIView* parentView;
    BilteralFilterCore *bilteralFilter;
    KeyFrameDetectCore *keyframeDector;
#if 0
    FILE *fout;
#endif
};

- (void)initIOSCapture;
- (int)start;
- (int)stop;
- (void)setSize:(MSVideoSize) size;
- (MSVideoSize*)getSize;
- (void)openDevice:(const char*) deviceId;
- (void)setFps:(float) value;
+ (Class)layerClass;

- (NSNumber*)registerOwner:(cloopenwebrtc::videocapturemodule::VideoCaptureiOS*)owner;
- (NSNumber*)setCaptureDeviceById:(char*)uniqueId;
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate;
- (NSNumber*)startCapture;
- (NSNumber*)stopCapture;
- (NSNumber*)updateLossRate:(int)lossRate;
- (void)focusWithMode:(AVCaptureFocusMode)focusMode exposeWithMode:(AVCaptureExposureMode)exposureMode atDevicePoint:(CGPoint)point monitorSubjectAreaChange:(BOOL)monitorSubjectAreaChange;
- (void)subjectAreaDidChange:(NSNotification *)notification;
- (void)hahaOnConnection:(NSNotification *)noti;
- (void)hahaOnDisconection:(NSNotification *)noti;

- (void)saveYUVtoFile:(unsigned char *)buf andwrap:(int)wrap andxsize:(int)xsize andysize:(int)ysize;
#ifdef __APPLE_CC__
- (void)setCaptureRotate:(VideoCaptureRotation)rotate;
#endif
- (BOOL) isAutoOrientation;
@property (nonatomic, retain, readonly) UIView* parentView;
@property (nonatomic, assign) bool triggered;
@property (nonatomic) dispatch_queue_t sessionQueue;
@end


static void capture_queue_cleanup(void* p) {
    ECIOSCaptureCCP *capture = (ECIOSCaptureCCP *)p;
    [capture release];
}
#endif