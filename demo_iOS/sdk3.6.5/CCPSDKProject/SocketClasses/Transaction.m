/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *                    http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "Transaction.h"

@implementation Transaction

@synthesize socket;
@synthesize tractionType;
@synthesize isHeaderComplete;
@synthesize status;
@synthesize writeMsgLen;
@synthesize aleardySendLen;
@synthesize contentLen;
@synthesize aleardyRcvLen;
@synthesize serverIp;
@synthesize serverPort;
@synthesize requestUrl;
@synthesize transData;
@synthesize isEnd;

- (void)dealloc
{
    self.socket = nil;
    self.serverIp = nil;
    self.serverPort = nil;
    self.requestUrl = nil;
    self.transData = nil;
    [super dealloc];
}

@end
