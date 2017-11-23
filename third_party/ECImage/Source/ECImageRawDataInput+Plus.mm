//
//  ECImageRawDataInput+Plus.m
//  ECImage
//
//  Created by 葛昭友 on 2017/11/20.
//  Copyright © 2017年 Brad Larson. All rights reserved.
//

#import "ECImageRawDataInput+Plus.h"
#include "libyuv.h"

@implementation ECImageRawDataInput (ECImageRawDataInputPlus)

/**
 * input: argb raw data.
 */
-(void)processARGBData:(GLubyte *)imageBytes imageSize:(CGSize)imageSize {
    [self updateDataFromBytes:imageBytes size:imageSize];
    [self processData];
}

/**
 * input: yuv i420 raw data.
 */
-(void)processI420Data:(GLubyte *)imageBytes imageSize:(CGSize)imageSize {
    //todo: convert i420toArgb
    int width  = imageSize.width;
    int height = imageSize.height;

    uint8_t* y_pointer = (uint8_t*)imageBytes;
    uint8_t* u_pointer = (uint8_t*)y_pointer + width*height;
    uint8_t* v_pointer = (uint8_t*)u_pointer + width*height/4;

    int y_stride = width;
    int u_stride = (width + 1) >> 1;
    int v_stride = (width + 1) >> 1;
    if(dst_argb == NULL) {
        dst_argb = (uint8_t*)malloc(width*height*4);
    }
    
    libyuv::I420ToARGB(y_pointer, y_stride, u_pointer, u_stride, v_pointer, v_stride, dst_argb, width*4, width, height);
    [self updateDataFromBytes:dst_argb size:imageSize];
    [self processData];
}
@end
