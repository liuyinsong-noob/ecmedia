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
#import <Foundation/Foundation.h>

typedef enum
{
    EChunked,
    ENormal
} ETractionType;

@interface Transaction : NSObject
{

}

//socket
@property (retain, nonatomic) id socket;
//消息类型
@property     ETractionType tractionType;
//判断消息头是否已经接收完
@property     Boolean isHeaderComplete;

@property     UInt16 status;
//要发送消息的消息长度,包括包头和包体
@property     UInt32 writeMsgLen;
//已发送的消息长度
@property     UInt32 aleardySendLen;

//将要接收包体的长度
@property     UInt32 contentLen;
//已接收的包体长度
@property     UInt32 aleardyRcvLen;


@property (retain, nonatomic) NSString *serverIp;
@property (retain, nonatomic) NSString *serverPort;
@property (retain, nonatomic) NSString *requestUrl;
@property (retain, nonatomic) id        transData;
@property (assign, nonatomic) BOOL      isEnd;
- (void)dealloc;

@end
