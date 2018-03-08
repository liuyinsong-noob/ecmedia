//
//  ECImageGreyFilter.m
//  ECImage
//
//  Created by 葛昭友 on 2018/3/7.
//  Copyright © 2018年 Brad Larson. All rights reserved.
//


#import "ECImageGreyFilter.h"
@implementation ECImageGreyFilter

#pragma mark -
#pragma mark Initialization and teardown

- (id)init;
{
    if (!(self = [super init]))
    {
        return nil;
    }
    
    [super setSaturation:0.0];
    return self;
}

@end
