//
//  ECImageFilterFactory.m
//  ECImage
//
//  Created by 葛昭友 on 2018/3/7.
//  Copyright © 2018年 Brad Larson. All rights reserved.
//

#import "image_filter_factory.h"

@implementation ECImageFilterFactory
    +(ECImageOutput<ECImageInput>*)createImageFiilterWithType:(yuntongxunwebrtc::ECImageFilterType) type {
        ECImageOutput<ECImageInput> *imageFilter = nullptr;
        switch (type) {
            case yuntongxunwebrtc::ECType_BeautyFaceFilter:
                imageFilter = [[ECImageBeautyFaceFilter alloc] init];
                break;
            case yuntongxunwebrtc::ECType_GrayFilter:
                imageFilter = [[ECImageGreyFilter alloc] init];
                break;
            default:
                imageFilter = [[ECImageFilter alloc] init];
                break;
        }
        
        return imageFilter;
    }
@end
