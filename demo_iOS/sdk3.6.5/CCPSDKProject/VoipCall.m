//
//  VoipCall.m
//  CCPSDKProject
//
//  Created by hubin  on 12-10-11.
//  Copyright (c) 2012年 hisunsray. All rights reserved.
//

#import "VoipCall.h"

@implementation VoipCall

@synthesize digID = _digID;
@synthesize callID = _callID;
@synthesize caller = _caller;
@synthesize callee = _callee;
@synthesize callDirect = _callDirect;
@synthesize callStatus = _callStatus;
@synthesize callType = _callType;

- (void)dealloc
{
    self.digID = nil;
    self.callID = nil;
    self.caller = nil;
    self.callee = nil;
    [super dealloc];    
}

@end
