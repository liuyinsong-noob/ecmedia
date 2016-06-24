//
//  VoipCall.h
//  CCPSDKProject
//
//  Created by hubin  on 12-10-11.
//  Copyright (c) 2012年 hisunsray. All rights reserved.
//

#import <Foundation/Foundation.h>

enum ECallDirect {
    EOutgoing = 0,
    EIncoming
    };

enum ECallStatus {
    ECallOutgoing = 0,      //外呼无应答
    ECallProceeding,        //外呼，服务器回100Tring
    ECallAlerting,          //外呼对方振铃
    ECallEarlyMedia,      //外呼早期媒体
    ECallFailedBusy,        //外呼失败
    ECallRing,              //来电振铃
    ECallStreaming,         //通话，外呼和来电
    ECallReleasing,         //释放呼叫请求中
    ECallPaused,            //呼叫保持
    ECallPausedByRemote,    //呼叫被保持
    ECallResumed,           //呼叫恢复
    ECallResumedByRemote,   //呼叫被恢复
    ECallTransfered,        //呼叫被转移
    ECallEnd,               //呼叫释放
};

@interface VoipCall : NSObject
{
    NSString *_digID;
    NSString *_callID;
    NSString *_caller;
    NSString *_callee;
    
    //0:outgoing   1:incoming
    int         _callDirect;
    //ECallStatus
    int         _callStatus;
    //0:voice   1:video
    int         _callType;
}
@property (retain) NSString *digID;
@property (retain) NSString *callID;
@property (retain) NSString *caller;
@property (retain) NSString *callee;
@property (assign) int callDirect;
@property (assign) int callStatus;
@property (assign) int callType;

@end
