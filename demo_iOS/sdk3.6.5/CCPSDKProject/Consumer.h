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
#import "CCPCallEvent.h"
#import "CommonClass.h"
@protocol  ConsumerSendChunkedDelegate<NSObject>

@optional
-(void) onRemoveSendTaskWithKey:(NSString*) key;
-(void) onsendChunkedWithReason:(NSInteger)reason andInstanceMsg:(InstanceMsg*) data;
-(void) onReadyToSendChunkedMsgWithFileName:(NSString*) fileName andReceiver:(NSString*) receiver andChunked:(BOOL) chunked andUserdata:(NSString*)userdata andSendMsgId:(NSString*)msgid;
@end


@interface Consumer : NSObject
{
    NSMutableSet *transSet;
    NSMutableArray* sendArray;
    int sendFlag;
}

@property(atomic,retain) NSCondition *condition;
@property (nonatomic,retain) NSTimer *workTimer;
@property(nonatomic,retain)  NSTimer *sendTimer;
@property(atomic,retain) NSMutableArray *products;
@property(nonatomic,retain) NSMutableArray *productsArray;
@property(atomic,retain) NSMutableDictionary *dictSend;
@property(nonatomic,assign) id<ConsumerSendChunkedDelegate> myDelegate;
@property(nonatomic,retain) NSString *fileName;
@property(nonatomic,retain) NSString *receiver;
@property(nonatomic,retain) NSString *userData;
@property(nonatomic,retain) NSString *MsgId;
-(void)sendChunkedFileWithVoiceMsg:(id) data andServerIP:(NSString *)serverIp withServerPort:(NSString *)serverPort byUrl:(NSString *)url;
-(void)createProductWithData:(id) data;
-(void)sendRest;
-(void)sendUploadFailedTimeIsShort;
@end
