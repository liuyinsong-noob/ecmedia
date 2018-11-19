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


#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>





@interface ECIOSDisplay : UIView {
    
    
@private
    // parent view
    // UIView* parentView;
    // is opengl rending frame
    BOOL isRunning;
    BOOL isRendering;
    //
    int deviceRotation;

    // OpenGL context
    EAGLContext             *_glContext;
    
    // frame buffer
    GLuint                  _framebuffer;
    
    // render buffer
    GLuint                  _renderBuffer;
    
    // shader program handler
    GLuint                  _program;
    
    // YUV texture array
    GLuint                  _textureYUV[3];
    
    // video width
    GLuint                  _videoW;
    
    // video height
    GLuint                  _videoH;
    
    // parent screen width
    GLuint                  _parentScreenW;
    
    // parent screen height
    GLuint                  _parentScreenH;
    
    // video preview scale factor
    GLsizei                 _viewScale;
    
    // record last contentmode
    UIViewContentMode       _lastContentMode;
    
    // record uiview width
    int _lastViewWidth;
    
    //record uiview height
    int _lastViewHeight;
@public
  
}

- (void) startRendering:(id)sender;
- (void) stopRendering:(id)ignore;
@property (nonatomic, retain) UIView* parentView;

#pragma mark - 接口
- (void)renderI420Frame:(void *)framebufer width:(NSInteger)width height:(NSInteger)height;

/**
 清除画面
 */
- (void)clearFrame;
@end
