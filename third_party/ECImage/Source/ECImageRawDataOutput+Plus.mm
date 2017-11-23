//
//  TTTsdf.m
//  ECImage
//
//  Created by 葛昭友 on 2017/11/20.
//  Copyright © 2017年 Brad Larson. All rights reserved.
//

#import "ECImageRawDataOutput+Plus.h"
#include "libyuv.h"
@implementation ECImageRawDataOutput(ECImageRawDataOutputPlus)
/**
 * add by zhaoyou
 */
-(void) commitBlock {
    __unsafe_unretained ECImageRawDataOutput * weakOutput = self;
    [self setNewFrameAvailableBlock:^{
        // invalid width or height
        if(imageSize.width == 0 || imageSize.width == 0) {
            return;
        }
        ///////////////////////
        [weakOutput lockFramebufferForReading];
        GLubyte *argbOutputBytes = [weakOutput rawBytesForImage];
        int bytesPerRow = (int)[weakOutput bytesPerRowInOutput];

        if(weakOutput.argbFrameAvailableBlock != nullptr) {
            weakOutput.argbFrameAvailableBlock(argbOutputBytes, bytesPerRow, weakOutput->imageSize.width, weakOutput->imageSize.height);
        }

        if(weakOutput.i420FrameAvailableBlock != nullptr) {
            int width   = weakOutput->imageSize.width;
            int height  = weakOutput->imageSize.height;

            if(_i420Bytes == NULL)
            {
                _i420Bytes = (uint8_t*)malloc(width*height*3/2);
            }

            uint8_t* y_pointer = weakOutput->_i420Bytes;
            uint8_t* u_pointer = (uint8_t*)y_pointer + width*height;
            uint8_t* v_pointer = (uint8_t*)u_pointer + width*height/4;
            int y_stride = width;
            int u_stride = (width + 1) >> 1;
            int v_stride = (width + 1) >> 1;

            // ARGB8888: every pixel 4Bytes, stride_argb = width * 4
            libyuv::ARGBToI420(argbOutputBytes, bytesPerRow, y_pointer, y_stride, u_pointer, u_stride, v_pointer, v_stride, width, height);
            weakOutput.i420FrameAvailableBlock(weakOutput->_i420Bytes, y_pointer, y_stride, u_pointer, u_stride, v_pointer, v_stride, width, height);
        }
        [weakOutput unlockFramebufferAfterReading];
    }];
}

-(void) setArgbFrameAvailableBlock:(ARGBFrameAvailableBlock)block {
    argbFrameAvailableBlock = block;
    if(i420FrameAvailableBlock == nullptr) {
        [self commitBlock];
    }
}

-(void) setI420FrameAvailableBlock:(I420FrameAvailableBlock)block {
    i420FrameAvailableBlock = block;
    if(argbFrameAvailableBlock == nullptr) {
        [self commitBlock];
    }
}
-(ARGBFrameAvailableBlock) argbFrameAvailableBlock {
    return argbFrameAvailableBlock;
}

-(I420FrameAvailableBlock) i420FrameAvailableBlock {
    return i420FrameAvailableBlock;
}

@end
