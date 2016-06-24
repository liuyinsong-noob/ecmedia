/*
 iosdisplay.m
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

#import <AVFoundation/AVFoundation.h>

#import "iosdisplay.h"
#include "scaler.h"
#include "opengles_display.h"

@interface ECIOSDisplay (PrivateMethods)
- (BOOL) loadShaders;
- (void) initGlRendering;
- (BOOL) isAutoOrientation;
@end

@implementation ECIOSDisplay

@synthesize imageView;

- (id)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self) {
        [self initGlRendering];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initGlRendering];
    }
    return self;
}

- (void)initGlRendering
{
    self->helper = ogl_display_new();
    
    // Initialization code
    CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;
    eaglLayer.opaque = TRUE;
    
    [self setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    
    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!context || ![EAGLContext setCurrentContext:context]) {
//        NSLog(@"Opengl context failure");
        return;
    }
    
    glGenFramebuffers(1, &defaultFrameBuffer);    
    glGenRenderbuffers(1, &colorRenderBuffer);
        
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderBuffer);

    // release GL context for this thread
    [EAGLContext setCurrentContext:nil];
    
    glInitDone = FALSE;
    allocatedW = allocatedH = 0;
    deviceRotation = 0;

    if ([self respondsToSelector:@selector(isAutoOrientation)]) {
        if ([self isAutoOrientation]) {
            UIDeviceOrientation oritentation = [[UIDevice currentDevice] orientation];
            if (oritentation == UIInterfaceOrientationPortrait ) {
                deviceRotation = 0;
            } else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
                deviceRotation = 270;
            } else if (oritentation == UIInterfaceOrientationLandscapeLeft ) {
                deviceRotation = 90;
            }else if(oritentation == UIInterfaceOrientationMaskAllButUpsideDown) {
                deviceRotation = 180;
            }
        }
    }
    
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver: self selector:   @selector(deviceOrientationNotify) name: UIDeviceOrientationDidChangeNotification object: nil];
}


- (void)deviceOrientationNotify {
    
    if ([self respondsToSelector:@selector(isAutoOrientation)]) {
        if ([self isAutoOrientation]) {
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
            
            if (deviceRotation != deviceOrientation) {
                deviceRotation = deviceOrientation;
            }
        }
    }
}

- (void) drawView:(id)sender
{
//    static int invokeCount = 0;
//    invokeCount++;
//    if (invokeCount%100 == 0) {
//        NSLog(@"drawView %d", invokeCount);
//    }
    /* no opengl es call made when in background */
    if ([UIApplication sharedApplication].applicationState !=  UIApplicationStateActive)
        return;
    
    @synchronized(self) {
        if (![EAGLContext setCurrentContext:context]) {
            //NSLog(@"Failed to bind GL context");
            return;
        }
        
        [self updateRenderStorageIfNeeded];
        
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);

        if (!glInitDone || !animating) {
            glClear(GL_COLOR_BUFFER_BIT);
        }else ogl_display_render(helper, deviceRotation, (int)self.contentMode);

        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);

        [context presentRenderbuffer:GL_RENDERBUFFER];
    }
}

- (void) updateRenderStorageIfNeeded
{
    @synchronized(self) {
    if (!(allocatedW == (int)self.frame.size.width && allocatedH == (int)self.frame.size.height)) {
        
        allocatedH = [self frame].size.height;
        allocatedW = [self frame].size.width;
        
        if (![EAGLContext setCurrentContext:context]) {
            //NSLog(@"Failed to set EAGLContext - expect issues");
        }
        glFinish();
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
        CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
        
        if (allocatedW != 0 || allocatedH != 0) {
            // release previously allocated storage
            [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:nil];
            allocatedW = allocatedH = 0;
        }
        // allocate storage
        if (![context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer]) {
            //NSLog(@"Error in renderbufferStorage (layer %p frame size: %f x %f)", layer, layer.frame.size.width, layer.frame.size.height);
        } else {
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &allocatedW);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &allocatedH);
            //NSLog(@"GL renderbuffer allocation size: %dx%d (layer frame size: %f x %f)", allocatedW, allocatedH, layer.frame.size.width, layer.frame.size.height);
            ogl_display_init(helper, self.frame.size.width, self.frame.size.height);
        
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
    }
    glInitDone = TRUE;
}

- (void) startRendering: (id)ignore
{
    if (!animating)
    {
        if (self.superview != self.imageView) {
            // remove from old parent
            [self removeFromSuperview];
            // add to new parent
            [self.imageView addSubview:self];
        }

        // schedule rendering
        displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];
        [displayLink setFrameInterval:4];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        animating = TRUE;
    }
}

- (void) stopRendering: (id)ignore
{
    if (animating)
    {
//        sean update begin 20131011 crash
        if (displayLink) {
            [displayLink invalidate];
        }
//    	[displayLink invalidate];
//        sean update end 20131011 crash
        [self removeFromSuperview];
        displayLink = nil;
        animating = FALSE;
        [self drawView:0];
    }
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

-(void) dealloc {
    [EAGLContext setCurrentContext:context];
    glFinish();
    ogl_display_uninit(helper, TRUE);
    ogl_display_free(helper);
    helper = NULL;
    [EAGLContext setCurrentContext:0];

    [context release];
    [imageView release];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#if ! __has_feature(objc_arc)
    [super dealloc];
#endif
}
@end

//static void iosdisplay_process(IOSDisplay* thiz, mblk_t *m){

//    
//    if (thiz != nil && m != nil) {
//        ogl_display_set_yuv_to_display(thiz->helper, m);
//    }
//    
//}


//static void iosdisplay_unit(IOSDisplay* thiz){
////    IOSDisplay* thiz=(IOSDisplay*)f->data;
//
//    [thiz performSelectorOnMainThread:@selector(stopRendering:) withObject:nil waitUntilDone:YES];
//    
//    [thiz release];
//}
//
///*filter specific method*/
///*  This methods declare the PARENT window of the opengl view.
//    We'll create on gl view for once, and then simply change its parent. 
//    This works only if parent size is the size in all possible orientation.
//*/
static int iosdisplay_set_native_window(ECIOSDisplay* thiz, void *arg) {
    UIView* parentView = *(UIView**)arg;
    
    if (thiz) {
//        NSLog(@"OpenGL view parent changed (%p -> %lu)", thiz.imageView, *((unsigned long*)arg));
        thiz.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
        [thiz performSelectorOnMainThread:@selector(stopRendering:) withObject:nil waitUntilDone:NO];
    } else if (parentView == nil) {
        return 0;
    } else {
        thiz = [[ECIOSDisplay alloc] initWithFrame:CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height)];
    }
    thiz.contentMode = parentView.contentMode;
    thiz.imageView = parentView;
    if (parentView)
        [thiz performSelectorOnMainThread:@selector(startRendering:) withObject:nil waitUntilDone:NO];

    return 0;
}
//
//    return 0;
//}
//
//static int iosdisplay_get_native_window(IOSDisplay* thiz, void *arg) {
////    IOSDisplay* thiz=(IOSDisplay*)f->data;
//    arg = &thiz->imageView;
//    return 0;
//}
//
//static int iosdisplay_set_device_orientation(IOSDisplay* thiz, void* arg) {
////    IOSDisplay* thiz=(IOSDisplay*)f->data;
//    if (!thiz)
//        return 0;
//    thiz->deviceRotation = *((int*)arg);
//    return 0;
//}
//
//static int iosdisplay_set_zoom(IOSDisplay* thiz, void* arg) {
////    IOSDisplay* thiz=(IOSDisplay*)f->data;
//    ogl_display_zoom(thiz->helper, arg);
//}


//static MSFilterMethod iosdisplay_methods[]={
//	{	MS_VIDEO_DISPLAY_SET_NATIVE_WINDOW_ID , iosdisplay_set_native_window },
//    {	MS_VIDEO_DISPLAY_GET_NATIVE_WINDOW_ID , iosdisplay_get_native_window },
//    {	MS_VIDEO_DISPLAY_SET_DEVICE_ORIENTATION,        iosdisplay_set_device_orientation },
//    {   MS_VIDEO_DISPLAY_ZOOM, iosdisplay_set_zoom},
//	{	0, NULL}
//};

//
//MSFilterDesc ms_iosdisplay_desc={
//	.id=MS_IOS_DISPLAY_ID, /* from Allfilters.h*/
//	.name="IOSDisplay",
//	.text="IOS Display filter.",
//	.category=MS_FILTER_OTHER,
//	.ninputs=2, /*number of inputs*/
//	.noutputs=0, /*number of outputs*/
//	.init=iosdisplay_init,
//	.preprocess=NULL,
//	.process=iosdisplay_process,
//	.postprocess=NULL,
//	.uninit=iosdisplay_unit,
//	.methods=iosdisplay_methods
//};
//MS_FILTER_DESC_EXPORT(ms_iosdisplay_desc)
