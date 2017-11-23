//
//  TTTsdf.h
//  ECImage
//
//  Created by 葛昭友 on 2017/11/20.
//  Copyright © 2017年 Brad Larson. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ECImageRawDataOutput.h"
@interface ECImageRawDataOutput(ECImageRawDataOutputPlus)
-(void) setArgbFrameAvailableBlock:(ARGBFrameAvailableBlock)block;
-(ARGBFrameAvailableBlock) argbFrameAvailableBlock;
-(void) setI420FrameAvailableBlock:(I420FrameAvailableBlock)block;
-(I420FrameAvailableBlock) i420FrameAvailableBlock;
@end
