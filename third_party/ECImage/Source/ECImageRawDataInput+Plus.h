//
//  ECImageRawDataInput+Plus.h
//  ECImage
//
//  Created by 葛昭友 on 2017/11/20.
//  Copyright © 2017年 Brad Larson. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ECImageRawDataInput.h"
@interface ECImageRawDataInput (ECImageRawDataInputPlus)
- (void)processARGBData:(GLubyte *)imageBytes imageSize:(CGSize)imageSize;
- (void)processI420Data:(GLubyte *)imageBytes imageSize:(CGSize)imageSize;
@end
