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
#if TARGET_OS_IPHONE
#import <OpenGLES/ES3/gl.h>
#else
#import <OpenGL/gl3.h>
#endif

//#if TARGET_OS_IPHONE
#define RTC_PIXEL_FORMAT GL_LUMINANCE

enum AttribEnum
{
    ATTRIB_VERTEX,
    ATTRIB_TEXTURE,
    ATTRIB_COLOR,
};

enum TextureType
{
    TEXY = 0,
    TEXU,
    TEXV,
    TEXC
};

//fragment shader program
#define FSH @"varying lowp vec2 TexCoordOut;\
\
uniform sampler2D SamplerY;\
uniform sampler2D SamplerU;\
uniform sampler2D SamplerV;\
\
void main(void)\
{\
mediump vec3 yuv;\
lowp vec3 rgb;\
\
yuv.x = texture2D(SamplerY, TexCoordOut).r;\
yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;\
yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;\
\
rgb = mat3( 1,       1,         1,\
0,       -0.39465,  2.03211,\
1.13983, -0.58060,  0) * yuv;\
\
gl_FragColor = vec4(rgb, 1);\
\
}"

// vertex shader program
#define VSH @"attribute vec4 position;\
attribute vec2 TexCoordIn;\
varying vec2 TexCoordOut;\
\
void main(void)\
{\
gl_Position = position;\
TexCoordOut = TexCoordIn;\
}"

@interface ECIOSDisplay (PrivateMethods)
- (BOOL) loadShaders;
- (void) initGlRendering;
- (BOOL) isAutoOrientation;

//config YUV Texture
- (void)configYUVTexture;

//load and init yuv shader
- (void)loadYUVShader;

// create frame and render buffer
- (BOOL)createFrameAndRenderBuffer;

// destory frame and render buffer
- (void)destoryFrameAndRenderBuffer;

/**
 compile shader program
 @param shaderCode        代码
 @param shaderType        类型
 @return 成功返回着色器 失败返回－1
 */
- (GLuint)compileShader:(NSString*)shaderCode withType:(GLenum)shaderType;

// render frame
- (void)doRenderFrame;

- (void)updatePreview:(id)sender;
@end




@implementation ECIOSDisplay

@synthesize parentView;
// start
- (void) startRendering: (id)sender
{
    if (!isRunning)
    {
        if (self.superview != self.parentView) {
            // remove from old parent
            _parentScreenW = self.parentView.bounds.size.width;
            _parentScreenH = self.parentView.bounds.size.height;
            [self removeFromSuperview];
            // add to new parent
            [self.parentView addSubview:self];
        }
        
        // [self updatePreview];
        isRunning = TRUE;
    }
}

- (void) stopRendering:(id)ignore
{
    if (isRunning)
    {
        [self removeFromSuperview];
        isRunning = FALSE;
    }
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

// 屏幕旋转
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

-(void) dealloc {
    [EAGLContext setCurrentContext:_glContext];
    glFinish();
    [EAGLContext setCurrentContext:0];
    [self removeFromSuperview];
    [_glContext release];
    [parentView release];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self removeObserver:self forKeyPath:@"frame" context:nil];
#if ! __has_feature(objc_arc)
    [super dealloc];
#endif
}

#pragma mark - INIT
- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        if (![self initOpengGL])
        {
            self = nil;
        }
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        if (![self initOpengGL])
        {
            self = nil;
        }
        [self addObserver:self forKeyPath:@"frame" options:NSKeyValueObservingOptionNew|NSKeyValueObservingOptionOld context:nil];
    }
    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context{
    if ([keyPath isEqualToString:@"frame"]) {
        _parentScreenW = self.parentView.bounds.size.width;
        _parentScreenH = self.parentView.bounds.size.height;
        [self updatePreview];
    }
}


- (BOOL)initOpengGL
{
    CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;

    // view opaque, TRUE: opaque
    eaglLayer.opaque = TRUE;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                    //[NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
                                    nil];
    
    [self setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [self registerNotification];
    isRendering = YES;
    
    // view scale
    self.contentScaleFactor = [UIScreen mainScreen].scale;
    _viewScale = [UIScreen mainScreen].scale;
    
    // alloc opengl version 2 context
    //  _glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    _glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    // set context
    if(!_glContext || ![EAGLContext setCurrentContext:_glContext]) {
        return NO;
    }
    
    // config texture
    [self configYUVTexture];
    // load shader
    [self loadYUVShader];
    
    glUseProgram(_program);
    
    //
    GLuint textureUniformY = glGetUniformLocation(_program, "SamplerY");
    GLuint textureUniformU = glGetUniformLocation(_program, "SamplerU");
    GLuint textureUniformV = glGetUniformLocation(_program, "SamplerV");
    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformU, 1);
    glUniform1i(textureUniformV, 2);
    
    // init lastContentMode
    _lastContentMode = UIViewContentModeScaleAspectFit;
    return YES;
}

- (void)configYUVTexture
{
    if (_textureYUV[TEXY]) {
        glDeleteTextures(3, _textureYUV);
    }
    glGenTextures(3, _textureYUV);
    if (!_textureYUV[TEXY] || !_textureYUV[TEXU] || !_textureYUV[TEXV]) {
        NSLog(@"create texture failed\n");
        return;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXY]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXU]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXV]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// load shader
- (void)loadYUVShader
{
    // compile vertext shader
    GLuint vertexShader = [self compileShader:VSH withType:GL_VERTEX_SHADER];
    // compile fragment shader
    GLuint fragmentShader = [self compileShader:FSH withType:GL_FRAGMENT_SHADER];
    
    // attach shader
    _program = glCreateProgram();
    glAttachShader(_program, vertexShader);
    glAttachShader(_program, fragmentShader);
    
    // bind program
    glBindAttribLocation(_program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(_program, ATTRIB_TEXTURE, "TexCoordIn");
    
    // link
    glLinkProgram(_program);
    
    // is link success
    GLint linkSuccess;
    glGetProgramiv(_program, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(_program, sizeof(messages), 0, &messages[0]);
        NSString *messageString = [NSString stringWithUTF8String:messages];
        NSLog(@"link shader failed:%@", messageString);
    }
    
    // delete shader
    if (vertexShader) {
        glDeleteShader(vertexShader);
    }
    
    if (fragmentShader) {
        glDeleteShader(fragmentShader);
    }
}

/**
 * compile shader
 */
- (GLuint)compileShader:(NSString*)shaderString withType:(GLenum)shaderType
{
    NSError *error;
    if (!shaderString) {
        NSLog(@"Error loading shader: %@", error.localizedDescription);
        exit(1);
    } else {
        // NSLog(@"shader code-->%@", shaderString);
    }
    
    // shader handle
    GLuint shaderHandle = glCreateShader(shaderType);
    
    // set shader source
    const char * shaderStringUTF8 = [shaderString UTF8String];
    int shaderStringLength = (int)[shaderString length];
    glShaderSource(shaderHandle, 1, &shaderStringUTF8, &shaderStringLength);
    
    // compile shader
    glCompileShader(shaderHandle);
    
    //
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        NSString *messageString = [NSString stringWithUTF8String:messages];
        NSLog(@"%@", messageString);
        return -1;
    }
    
    return shaderHandle;
}

#pragma mark - render i420 frame
/**
 * render i420 frame
 *
 * @framebuffer: i420 buffer
 * @width:       frame width
 * @height:      frame height
 */
- (void)renderI420Frame:(void *)framebufer width:(NSInteger)width height:(NSInteger)height
{
    if (UIApplicationStateActive == [UIApplication sharedApplication].applicationState) {
        isRendering = YES;
    }
    else {
        isRendering = NO;
        return;
    }
    // render when view ready
    if (!self.window) {
        return;
    }
    if(!isRendering) {
        return;
    }
    
    @synchronized(self)
    {
        if (width != _videoW || height != _videoH) {
            [self setVideoSize:width height:height];
        }
        
        // texture setting
        [EAGLContext setCurrentContext:_glContext];
        glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXY]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_EXT, GL_UNSIGNED_BYTE, framebufer);
        
        glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXU]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)framebufer + width * height);
        
        glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXV]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)framebufer + width * height * 5 / 4);
        
        // render frame
        [self doRenderFrame];
    }
}
- (void)renderI420Frame2:(yuntongxunwebrtc::I420VideoFrame &)videoFrame width:(NSInteger)width height:(NSInteger)height
{
    if (UIApplicationStateActive == [UIApplication sharedApplication].applicationState) {
        isRendering = YES;
    }
    else {
        isRendering = NO;
        return;
    }
    // render when view ready
    if (!self.window) {
        return;
    }
    if(!isRendering) {
        return;
    }
    
    @synchronized(self)
    {
        if (width != _videoW || height != _videoH) {
            [self setVideoSize:videoFrame];
        }
        bool strid_eq_width = !(width == videoFrame.stride(kYPlane));
        // texture setting
        [EAGLContext setCurrentContext:_glContext];
        glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXY]);
        // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_EXT, GL_UNSIGNED_BYTE, videoFrame.buffer(kYPlane));
       strid_eq_width ? glPixelStorei(GL_UNPACK_ROW_LENGTH, videoFrame.stride(kYPlane)) : void();
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     RTC_PIXEL_FORMAT,
                     static_cast<GLsizei>(width),
                     static_cast<GLsizei>(height),
                     0,
                     RTC_PIXEL_FORMAT,
                     GL_UNSIGNED_BYTE,
                     videoFrame.buffer(kYPlane));
        strid_eq_width ? glPixelStorei(GL_UNPACK_ROW_LENGTH, 0) : void();
        
        strid_eq_width = !(width/2 == videoFrame.stride(kUPlane));
        glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXU]);
        // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (width)/2, (height)/2, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)videoFrame.buffer(kUPlane));
        
        strid_eq_width ? glPixelStorei(GL_UNPACK_ROW_LENGTH, videoFrame.stride(kUPlane)) : void();
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     RTC_PIXEL_FORMAT,
                     static_cast<GLsizei>(width/2),
                     static_cast<GLsizei>(height/2),
                     0,
                     RTC_PIXEL_FORMAT,
                     GL_UNSIGNED_BYTE,
                     videoFrame.buffer(kUPlane));
         strid_eq_width ? glPixelStorei(GL_UNPACK_ROW_LENGTH, 0) : void();
        
        
        glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXV]);
        // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (width)/2, (height)/2, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)videoFrame.buffer(kVPlane));
        strid_eq_width ? glPixelStorei(GL_UNPACK_ROW_LENGTH, videoFrame.stride(kVPlane)) : void();
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     RTC_PIXEL_FORMAT,
                     static_cast<GLsizei>(width/2),
                     static_cast<GLsizei>(height/2),
                     0,
                     RTC_PIXEL_FORMAT,
                     GL_UNSIGNED_BYTE,
                     videoFrame.buffer(kVPlane));
        strid_eq_width ?  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0) : void();
        
        
        
        // render frame
        [self doRenderFrame];
    }
}
- (void)setVideoSize:(yuntongxunwebrtc::I420VideoFrame &)videoFrame
{
    int width = videoFrame.width();
    int height = videoFrame.height();
    
    int stride_y = videoFrame.stride(kYPlane);
    int stride_u = videoFrame.stride(kUPlane);
    int stride_v = videoFrame.stride(kVPlane);
    float aspect_ratio_old = _videoW/_videoH;
    float aspect_ratio_new = width/height;
    
    
    if(_videoW > 1 && _videoH > 1 && aspect_ratio_new != aspect_ratio_old){
        [self clearFrame];
    }
    _videoW = width;
    _videoH = height;
    
    // I420 frame size : width * height * 3/2
    void *frameBuffer = malloc(stride_y * height +  stride_u* height/2 + stride_v *height/2 + 1);
    if(frameBuffer) {
        memset(frameBuffer, 0x0, stride_y * height + stride_u* height/2 + stride_v *height/2+ 1) ;
    }
    
    [EAGLContext setCurrentContext:_glContext];
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXY]);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width, height, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, videoFrame.buffer(kYPlane));
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXU]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, (width)/2, (height)/2, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)videoFrame.buffer(kUPlane) );
    
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXV]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, (width)/2, (height)/2, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)videoFrame.buffer(kVPlane)  );
    free(frameBuffer);
}

- (void)setVideoSize:(GLuint)width height:(GLuint)height
{
    
    float aspect_ratio_old = _videoW/_videoH;
    float aspect_ratio_new = width/height;
    
    
    if(_videoW > 1 && _videoH > 1 && aspect_ratio_new != aspect_ratio_old){
        [self clearFrame];
    }
    _videoW = width;
    _videoH = height;
    // I420 frame size : width * height * 3/2
    void *frameBuffer = malloc(width * height * 1.5);
    if(frameBuffer) {
        memset(frameBuffer, 0x0, width * height * 1.5);
    }
    
    [EAGLContext setCurrentContext:_glContext];
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXY]);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width, height, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, frameBuffer);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXU]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width/2, height/2, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)frameBuffer + width * height);
    
    glBindTexture(GL_TEXTURE_2D, _textureYUV[TEXV]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width/2, height/2, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, (uint8_t *)frameBuffer + width * height* 5/4);
    free(frameBuffer);
}

- (void)doRenderFrame
{
    // 防止UIView缩放模式由拉伸模式变为比例缩放模式后, UIView周围还残留之前图像
    if(_lastContentMode != self.contentMode) {
        _lastContentMode = self.contentMode;
      //  [self clearFrame];
    }
    
    // 防止UIView尺寸又大变小, UIView周围还残留之前图像
    if((_lastViewWidth != (int)(self.parentView.bounds.size.width)) || (_lastViewHeight != (int)(self.parentView.bounds.size.height))) {
        _lastViewWidth = self.parentView.bounds.size.width;
        _lastViewHeight = self.parentView.bounds.size.height;
        _parentScreenH = _lastViewHeight;
        _parentScreenW = _lastViewWidth;
        dispatch_async(dispatch_get_main_queue(), ^{
            @synchronized(self)
            {
                self.frame = self.parentView.bounds;
            }
        });
        
        //  [self clearFrame];
    }
    
    [EAGLContext setCurrentContext:_glContext];
    CGSize size = self.bounds.size;
    
    int vpx = 0, vpy = 0, vpw = 1, vph = 1;
    int x=vpx,y=vpy,w=0,h=0;
    // Fill the smallest dimension, then compute the other one using the image ratio
    if (UIViewContentModeScaleAspectFit == self.contentMode) {
        if (_parentScreenW <= _parentScreenH) {
            float ratio = (_videoH) / (float)(_videoW);
            w = _parentScreenW * vpw;
            h = w * ratio;
            if (h > _parentScreenH) {
                w *= _parentScreenH /(float) h;
                h = _parentScreenH;
            }
            x = vpx * _parentScreenW;
            y = vpy * _parentScreenH;
        } else {
            
            float ratio = _videoW / (float)_videoH;
            h = _parentScreenH * vph;
            w = h * ratio;
            if (w > _parentScreenW) {
                h *= _parentScreenW / (float)w;
                w = _parentScreenW;
            }
            x = vpx * _parentScreenW;
            y = vpy * _parentScreenH;
            
        }
    } else if (UIViewContentModeScaleAspectFill == self.contentMode) {
        if (_parentScreenW <= _parentScreenH) {
            float ratio = _videoW / (float)_videoH;
            h = _parentScreenH * vph;
            w = h * ratio;
            x = vpx * _parentScreenW;
            y = vpy * _parentScreenH;
        } else {
            float ratio = _videoH / (float)(_videoW);
            w = _parentScreenW * vpw;
            h = w * ratio;
            x = vpx * _parentScreenW;
            y = vpy * _parentScreenH;
        }
    } else {
        if (_videoH>0 && _videoW>0) {
            w = _parentScreenW;
            h = _parentScreenH;
        }
    }
    
    // static GLfloat squareVertices[8];
    GLfloat squareVertices[8];
    
    squareVertices[0] = (float)(x - w) / _parentScreenW - 0.;
    squareVertices[1] = (float)(y - h) / _parentScreenH - 0.;
    squareVertices[2] = (float)(x + w) / _parentScreenW - 0.;
    squareVertices[3] = (float)(y - h) / _parentScreenH - 0.;
    squareVertices[4] = (float)(x - w) / _parentScreenW - 0.;
    squareVertices[5] = (float)(y + h) / _parentScreenH - 0.;
    squareVertices[6] = (float)(x + w) / _parentScreenW - 0.;
    squareVertices[7] = (float)(y + h) / _parentScreenH - 0.;
    
    GLfloat coordVertices[] = {
        0.0f,   1.0f,
        1.0f,   1.0f,
        0.0f,   0.0f,
        1.0f,   0.0f,
    };
    if (UIApplicationStateActive == [UIApplication sharedApplication].applicationState) {
        isRendering = YES;
    }
    else {
        isRendering = NO;
        return;
    }
    // coordinate transformation
    glViewport(0, 0, _parentScreenW*_viewScale, _parentScreenH*_viewScale);
    
    // update attribute values
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, coordVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
    
    // draw frame buffer
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    
    [_glContext presentRenderbuffer:GL_RENDERBUFFER];
}

// not call
- (void)clearFrame
{
    if (UIApplicationStateActive == [UIApplication sharedApplication].applicationState) {
        isRendering = YES;
    }
    else {
        isRendering = NO;
        return;
    }
    if ([self window])
    {
        [EAGLContext setCurrentContext:_glContext];
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
        [_glContext presentRenderbuffer:GL_RENDERBUFFER];
    }
}

#pragma mark - updatePreview
- (void)updatePreview
{
    dispatch_async(dispatch_get_main_queue(), ^{
        @synchronized(self)
        {
            [self clearFrame];
            [EAGLContext setCurrentContext:_glContext];
            [self destoryFrameAndRenderBuffer];
            [self createFrameAndRenderBuffer];
        }
        glViewport(0, 0, self.bounds.size.width*_viewScale, self.bounds.size.height*_viewScale);
    });
}

- (BOOL)createFrameAndRenderBuffer
{
    // create and bind frame buffer
    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    // create and bind render buffer
    glGenRenderbuffers(1, &_renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    // attach buffer
    if (![_glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer]) {
        NSLog(@"attach render buffer failed");
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        NSLog(@"create buffer failed 0x%x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return NO;
    }
    return YES;
}

- (void)destoryFrameAndRenderBuffer
{
    if (_framebuffer) {
        glDeleteFramebuffers(1, &_framebuffer);
    }
    
    if (_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
    }
    
    _framebuffer = 0;
    _renderBuffer = 0;
}

- (void)appWillResignActive:(NSNotification *)noti
{
    @synchronized(self)
    {
        isRendering = NO;
        glFinish();
    }
}

- (void)appDidEnterBackgroundFun:(NSNotification*)noti
{
    @synchronized(self)
    {
        isRendering = NO;
        glFinish();
    }
}

- (void)appWillEnterForeground:(NSNotification *)noti
{
    isRendering = YES;
}

-(void)appWillBecomeActive:(NSNotification *)noti {
    isRendering = YES;
}

-(void)registerNotification {
    // register notification
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appDidEnterBackgroundFun:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
}

@end
