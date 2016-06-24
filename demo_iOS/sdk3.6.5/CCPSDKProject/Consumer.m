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

#import "Consumer.h"
#import "SocketClasses/AsyncSocket.h"
#import "SocketClasses/Transaction.h"
#import "CCPRestService.h"
#define TIMEOUTINTERVAL 30


@implementation Consumer
@synthesize condition;
@synthesize products;
@synthesize productsArray;
@synthesize myDelegate;
@synthesize workTimer;
@synthesize fileName;
@synthesize receiver;
@synthesize userData;
@synthesize MsgId;

-(id)init;
{
    self = [super init];
    if (self)
    {
        NSCondition* tmp1 = [[NSCondition alloc] init];
        self.condition = tmp1;
        [tmp1 release];
        
        NSMutableArray *tmp2 = [[NSMutableArray alloc] init];
        self.products = tmp2;
        [tmp2 release];
        
        NSMutableArray *tmp3 = [[NSMutableArray alloc] init];
        self.productsArray = tmp3;
        [tmp3 release];
        
        
        transSet = [[NSMutableSet alloc] init];
        sendArray = [[NSMutableArray alloc] init];
        sendFlag = 0;
        
        NSMutableDictionary*tmp4 = [[NSMutableDictionary alloc] init];
        self.dictSend = tmp4;
        [tmp4 release];
        
        [NSThread detachNewThreadSelector:@selector(Consumption) toTarget:self withObject:nil];
        self.workTimer = [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(sendDataArray) userInfo:nil repeats:YES];
    }
    return self;
}

-(void)sendRest
{
    if(myDelegate
       && [myDelegate respondsToSelector:@selector(onReadyToSendChunkedMsgWithFileName:andReceiver:andChunked:andUserdata:andSendMsgId:)])
    {
        [myDelegate onReadyToSendChunkedMsgWithFileName:self.fileName andReceiver:self.receiver andChunked:YES andUserdata:self.userData andSendMsgId:self.MsgId];
    }
}
-(void)createProductWithData:(id) data
{
    [condition lock];
    [products addObject: data];
    [condition signal];
    [condition unlock];
}
-(void)sendUploadFailedTimeIsShort
{
    IMAttachedMsg* msg = [[IMAttachedMsg alloc] init];
    msg.fileUrl = self.fileName;    
    if(myDelegate
       && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
    {
        [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailedTimeIsShort andInstanceMsg:msg];
    }
    [msg release];
}
-(void)sendDataArray
{
    if (sendFlag == 0 && [sendArray count] > 0)
    {
        [self.workTimer invalidate];
        self.workTimer = nil;
        NSMutableDictionary* dict = [sendArray objectAtIndex:0];
        Transaction * trans = [self createTransByType:EChunked];
        trans.serverIp = [dict objectForKey:@"serverIp"];
        trans.serverPort = [dict objectForKey:@"serverPort"];
        trans.requestUrl = [dict objectForKey:@"url"];
        trans.transData = [dict objectForKey:@"data"];
        [sendArray removeObjectAtIndex:0];
        NSError *err = nil;
        sendFlag = 1;
        if(![trans.socket connectToHost:trans.serverIp onPort:trans.serverPort.intValue error:&err])
        {
            [self.workTimer invalidate];
            self.workTimer = nil;
            InstanceMsg* msg = trans.transData;
            if(myDelegate
               && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
            {
                [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
            }
            [self closeTransaction:EChunked];
        }
    }
}
-(void)Consumption
{
    while (YES)
    {
        usleep(10000);//10毫秒
        [condition lock];
        if ([products count] == 0) {
            [condition wait];
        }
        NSDictionary* dict = [products objectAtIndex:0];
        [productsArray addObject:dict];
        [products removeObjectAtIndex:0];
        [condition unlock];
    }
}

#pragma mark - delegate fun
- (void)sendChunkedFileWithVoiceMsg:(id) data andServerIP:(NSString *)serverIp withServerPort:(NSString *)serverPort byUrl:(NSString *)url
{
    NSMutableDictionary* dict =[[NSMutableDictionary alloc] init];
    [dict setObject:data forKey:@"data"];
    [dict setObject:serverIp forKey:@"serverIp"];
    [dict setObject:serverPort forKey:@"serverPort"];
    [dict setObject:url forKey:@"url"];
    [sendArray addObject:dict];
    [dict release];
}

#pragma mark - 构造消息，组建包头和包体
- (CFDataRef)getBodyByTrans:(Transaction *)trans
{
    NSString *oriBody = nil;
    
    switch (trans.tractionType)
    {

            break;
        default:
        {
            return nil;
        }
            break;
    }
    
    NSData *mybody = [oriBody dataUsingEncoding:NSASCIIStringEncoding];
    
    return (CFDataRef)mybody;
    
}

- (CFHTTPMessageRef)buildRequest:(Transaction*)trans
{
    if(!trans)
        return nil;
    
    NSString *requestMethod = [[NSString alloc] initWithString:@"POST"];
    NSURL *requestUrl = [NSURL URLWithString:trans.requestUrl];
    
    CFHTTPMessageRef requestMsg;
    requestMsg = CFHTTPMessageCreateRequest( kCFAllocatorDefault,
                                            (CFStringRef)requestMethod,
                                            (CFURLRef)requestUrl,
                                            kCFHTTPVersion1_1);
    [requestMethod release];
    
    if(!requestMsg)
        return nil;
    
    NSString *strHost = [NSString stringWithFormat:@"%@:%@", trans.serverIp, trans.serverPort];
    CFHTTPMessageSetHeaderFieldValue(requestMsg,
                                     (CFStringRef)@"Host",
                                     (CFStringRef)strHost);
    
    ETractionType type = trans.tractionType;
    
    CFHTTPMessageSetHeaderFieldValue(requestMsg,
                                     (CFStringRef)@"Content-Type",
                                     (CFStringRef)@"application/octet-stream");
    
    if(type == EChunked)
    {
        CFHTTPMessageSetHeaderFieldValue(requestMsg, (CFStringRef)@"Transfer-Encoding", (CFStringRef)@"chunked");
        CFHTTPMessageSetHeaderFieldValue(requestMsg,(CFStringRef)@"Accept",(CFStringRef)@"application/xml");
        CFHTTPMessageSetHeaderFieldValue(requestMsg,(CFStringRef)@"Connection",(CFStringRef)@"Keep-Alive");
    }
    else
    {
        CFDataRef data = [self getBodyByTrans:trans];
        CFHTTPMessageSetBody(requestMsg, data);
        //  Add the message length to the HTTP message header.
        CFHTTPMessageSetHeaderFieldValue(requestMsg,
                                         (CFStringRef)@"Content-Length",
                                         (CFStringRef) [NSString stringWithFormat: @"%ld",CFDataGetLength(data)]);
    }
    return requestMsg;
}

//- (CFHTTPMessageRef)buildResponse:(Transaction *)trans
//{
//    if(!trans)
//        return nil;
//}

#pragma mark - 创建 关闭Transaction

- (Transaction *)createTransByType:(ETractionType)type
{
    Transaction *trans = [Transaction alloc];
    id socket = nil;
    
    socket =  [[AsyncSocket alloc] initWithDelegate:self userData:(long)trans];
    trans.socket = socket;
    trans.tractionType = type;
    [transSet addObject:trans];
    [socket release];
    return trans;
}

- (Boolean)closeTransaction:(ETractionType)type
{
    [self.sendTimer invalidate];
    self.sendTimer = nil;
    Transaction *trans;
    for (trans in transSet)
    {
        if(trans.tractionType == type)
            break;
    }

    if(!trans)
        return NO;

    [trans.socket close];
    [transSet removeObject:trans];
    [trans release];

    return YES;
}

-(void)setFlagZero
{
    sendFlag = 0;
}
#pragma mark - AsyncSocketDelegate

- (void)onSocket:(AsyncSocket *)sock willDisconnectWithError:(NSError *)err
{
    
}

- (void)onSocketDidDisconnect:(AsyncSocket *)sock
{
    [self setFlagZero];
}

- (void)onSocket:(AsyncSocket *)sock didConnectToHost:(NSString *)host port:(UInt16)port
{
    Transaction *trans = nil;
    CFHTTPMessageRef requestMsg = nil;
    NSData *requestData = nil;
    trans = (Transaction *)sock.userData;
    requestMsg = [self buildRequest:trans];
    requestData = (NSData *) CFHTTPMessageCopySerializedMessage(requestMsg);    
    //debug
    NSString *reqAsString = [[NSString alloc] initWithData:requestData encoding:NSUTF8StringEncoding];
    //NSLog(@"SendMsg=%@", reqAsString);
    [reqAsString release];
    [sock writeData:requestData withTimeout:TIMEOUTINTERVAL tag:123456789];
    //
    self.sendTimer = [NSTimer scheduledTimerWithTimeInterval:.1 target:self selector:@selector(sendChunkedFileData:) userInfo:sock repeats:YES];
    CFRelease(requestMsg);
    [requestData release];
}

-(void)sendChunkedFileData:(NSTimer*)timer
{
    AsyncSocket *sock = [timer userInfo];
    Transaction *trans =nil;
    trans = (Transaction *)sock.userData;
    if(!trans || sendFlag ==0)
    {
        [self closeTransaction:EChunked];
        return;
    }

    //while (YES)
    {        
        if ([self.productsArray count] <= 0)
        {
            return;
        }
        IMAttachedMsg* msg = trans.transData;
        NSDictionary* dict = [self.productsArray objectAtIndex:0];
        if (![[dict objectForKey:@"file"] isEqualToString:msg.fileUrl])
            return;
        if ([[dict objectForKey:@"state"] isEqualToString:@"cancel"])
        {
            [timer invalidate];
            //NSLog(@"send cancel");
            //NSLog(@"send imFile is %@  ----file is%@ " ,msg.fileUrl ,[dict objectForKey:@"file"]);
            NSString *strBody = [NSString stringWithFormat: @"%x\r\n\r\n",0];
            NSData* dataBody = [strBody dataUsingEncoding:NSASCIIStringEncoding];
            [sock writeData:dataBody withTimeout:TIMEOUTINTERVAL tag:123456789];
            if(myDelegate
               && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
            {
                [myDelegate onsendChunkedWithReason:ELocalReason_UploadCancel andInstanceMsg:msg];
            }
            [self closeTransaction:EChunked];
        }
        else
        if ([[dict objectForKey:@"state"] isEqualToString:@"end"])
        {
            [timer invalidate];
            //NSLog(@"send end");
            //NSLog(@"send imFile is %@  ----file is%@ ",msg.fileUrl ,[dict objectForKey:@"file"]);
            if(!trans)
            {
                [self closeTransaction:EChunked];
                return;
            }
            NSData* data =(NSData*)[dict objectForKey:@"data"];
            int size = [data length];
            if (size > 0)
            {
                NSMutableData* mData = [[NSMutableData alloc] init];
                NSData* data =(NSData*)[dict objectForKey:@"data"];
                int size = [data length];
                trans.writeMsgLen += size;
                [mData appendData:[[NSString stringWithFormat:@"%x\r\n",size] dataUsingEncoding:NSASCIIStringEncoding]];
                [mData appendData:data];
                [mData appendData:[@"\r\n" dataUsingEncoding:NSASCIIStringEncoding]];
                [sock writeData:mData withTimeout:TIMEOUTINTERVAL tag:123456789];
                [mData release];
            }
            if (trans.writeMsgLen < 650)
            {
                NSString *strBody = [NSString stringWithFormat: @"%x\r\n\r\n",0];
                NSData* dataBody = [strBody dataUsingEncoding:NSASCIIStringEncoding];
                [sock writeData:dataBody withTimeout:TIMEOUTINTERVAL tag:123456789];                
                [self.productsArray removeObjectAtIndex:0];
               
                if(myDelegate
                   && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
                {
                    [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailedTimeIsShort andInstanceMsg:msg];
                }
                
                [self closeTransaction:EChunked];
                return;
            }
            else
            {                
                NSString *strBody = @"0B\r\n#!HISUNSTOP\r\n";
                NSData* dataBody = [strBody dataUsingEncoding:NSUTF8StringEncoding];
                [sock writeData:dataBody withTimeout:TIMEOUTINTERVAL tag:123456789];
            }
            NSString *strBody = [NSString stringWithFormat: @"%x\r\n\r\n",0];
            NSData* dataBody = [strBody dataUsingEncoding:NSASCIIStringEncoding];
            [sock writeData:dataBody withTimeout:TIMEOUTINTERVAL tag:123456789];
            trans.isEnd = YES;
        }
        else if ([[dict objectForKey:@"state"] isEqualToString:@"begin"])
        {
            //NSLog(@"send begin");
            //NSLog(@"send imFile is %@  ----file is%@ ",msg.fileUrl ,[dict objectForKey:@"file"]);
            trans.writeMsgLen = 0;
            NSString *strBody = [NSString stringWithFormat: @"%x\r\n#!AMR\n\r\n",6];
            NSData* dataBody = [strBody dataUsingEncoding:NSASCIIStringEncoding];
            [sock writeData:dataBody withTimeout:TIMEOUTINTERVAL tag:123456789];
        }
        else
        {            
            NSMutableData* mData = [[NSMutableData alloc] init];
            NSData* data =(NSData*)[dict objectForKey:@"data"];
            int size = [data length];
            trans.writeMsgLen += size;
//            NSLog(@"send %x imFile is %@  ----file is%@ ",size,msg.fileUrl ,[dict objectForKey:@"file"]);
            [mData appendData:[[NSString stringWithFormat:@"%x\r\n",size] dataUsingEncoding:NSASCIIStringEncoding]];
            [mData appendData:data];
            [mData appendData:[@"\r\n" dataUsingEncoding:NSASCIIStringEncoding]];
            [sock writeData:mData withTimeout:TIMEOUTINTERVAL tag:123456789];
            [mData release];
        }
        [self.productsArray removeObjectAtIndex:0];
    }
}
- (void)onSocket:(AsyncSocket *)sock didReadData:(NSData *)data withTag:(long)tag
{
    Transaction *trans = nil;
    trans = (Transaction *)sock.userData;
    //首先处理消息头
    if(!trans.isHeaderComplete)
    {
        CFHTTPMessageRef httpresponse = CFHTTPMessageCreateEmpty(kCFAllocatorDefault, NO);
        CFHTTPMessageAppendBytes(httpresponse, [data bytes], [data length]);
        
        trans.isHeaderComplete = CFHTTPMessageIsHeaderComplete(httpresponse);
        
        if(trans.isHeaderComplete)
        {
            trans.isHeaderComplete = YES;
            
            //接收到的响应消息
            CFStringRef slength = CFHTTPMessageCopyHeaderFieldValue(httpresponse, (CFStringRef)@"Content-Length");
            trans.contentLen = [(NSString *)slength doubleValue];
            trans.status = CFHTTPMessageGetResponseStatusCode(httpresponse);            
            if(trans.status == 200)
            {                
                if(trans.contentLen > 0)
                {
                    NSString *content = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                    NSLog(@"header1 = %@", content);
                    NSString *tmp2 = @"0";
                    NSRange range1 = [content rangeOfString:@"<statusCode>"];
                    if (range1.length > 0)
                    {
                        NSString *tmp1 = [content substringFromIndex:range1.location+range1.length];
                        NSRange range2 = [tmp1 rangeOfString:@"</statusCode>"];
                        if (range2.length >0) {
                            tmp2 = [tmp1 substringToIndex:range2.location];
                        }
                        else
                            NSLog(@"content error no find statusCode");
                    }
                     InstanceMsg* msg = trans.transData;
                    if (tmp2.intValue == 0)
                    {
                        if(myDelegate
                           && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
                        {
                            [myDelegate onsendChunkedWithReason:0 andInstanceMsg:msg];
                        }
                    }
                    else
                    {                        
                        if(myDelegate
                           && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
                        {
                            [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
                        }
                    }                    
                    [self closeTransaction:trans.tractionType];
                    
                    [content release];
                }
                else
                {
                    //现在响应消息都有包体，如果响应没包体，认为是失败
                    InstanceMsg* msg = trans.transData;
                    if(myDelegate
                       && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
                    {
                        [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
                    }
                    [self closeTransaction:trans.tractionType];
                }
            }
            else
            {
                //现在所有响应消息都是２００　OK，否则认为是失败
                InstanceMsg* msg = trans.transData;
                if(myDelegate
                   && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
                {
                    [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
                }
                [self closeTransaction:EChunked];
            }
            
            if ( slength != nil )
            {
                CFRelease( slength );
            }
        }
        
        CFRelease(httpresponse);
    }
    else
    {
        //处理包体，现在错误消息也在包体里面
        trans.isHeaderComplete = NO;        
        
        InstanceMsg* msg = trans.transData;
        if(myDelegate
           && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
        {
            [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
        }
        [self closeTransaction:EChunked];
    }
}

- (void)onSocket:(AsyncSocket *)sock didReadPartialDataOfLength:(NSUInteger)partialLength tag:(long)tag
{
    
}

- (void)onSocket:(AsyncSocket *)sock didWriteDataWithTag:(long)tag
{
    Transaction *trans =nil;
    trans = (Transaction *)sock.userData;
    
    if(trans)
    {
        if (trans.isEnd)
        {
            [sock readDataWithTimeout:TIMEOUTINTERVAL tag:0];
        }
    }
}

- (void)onSocket:(AsyncSocket *)sock didWritePartialDataOfLength:(NSUInteger)partialLength tag:(long)tag
{
    
}

- (NSTimeInterval)onSocket:(AsyncSocket *)sock
  shouldTimeoutReadWithTag:(long)tag
                   elapsed:(NSTimeInterval)elapsed
                 bytesDone:(NSUInteger)length
{
    NSLog(@"shouldTimeoutReadWithTag");
    Transaction *trans =nil;
    trans = (Transaction *)sock.userData;
    if(trans)
    {        
        InstanceMsg* msg = trans.transData;
        if(myDelegate
           && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
        {
            [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
        }
        
        [self closeTransaction:EChunked];
        if(!trans)
            return 0;
    }
    return 0;
}

- (NSTimeInterval)onSocket:(AsyncSocket *)sock
 shouldTimeoutWriteWithTag:(long)tag
                   elapsed:(NSTimeInterval)elapsed
                 bytesDone:(NSUInteger)length
{
    NSLog(@"shouldTimeoutWriteWithTag");
    Transaction *trans =nil;
    trans = (Transaction *)sock.userData;
    if(trans)
    {
        InstanceMsg* msg = trans.transData;
        if(myDelegate
           && [myDelegate respondsToSelector:@selector(onsendChunkedWithReason:andInstanceMsg:)])
        {
            [myDelegate onsendChunkedWithReason:ELocalReason_UploadFailed andInstanceMsg:msg];
        }

        [self closeTransaction:EChunked];
        if(!trans)
            return 0;
    }
    return 0;
}

- (void)onSocketDidSecure:(AsyncSocket *)sock
{
    
}


-(void)dealloc
{
    [self closeTransaction:EChunked];
    self.products = nil;
    self.condition = nil;
    [sendArray release];
    self.productsArray = nil;
    [transSet release];
    self.myDelegate = nil;
    [self.sendTimer invalidate];
    self.sendTimer = nil;
    [self.workTimer invalidate];
    self.workTimer = nil;
    self.dictSend = nil;
    self.fileName = nil;
    self.receiver = nil;
    self.userData = nil;
    self.MsgId = nil;
    [super dealloc];
}
@end
