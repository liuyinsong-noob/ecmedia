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

enum ECryptType{
    AES_256_SHA1_80 = 3,
    AES_256_SHA1_32
};

#define _custom_method_version_

#import <sys/utsname.h>
#import "CCPCallService.h"
#import <UIKit/UIKit.h>
#import "CommonClass.h"
#include <AVFoundation/AVAudioSession.h>

#import "CCPCallEvent.h"
#import "CCPClient.h"
#import "CCPClient_Internal.h"
#import "Reachability/TReachability.h"
#import "VoipCall.h"
#import "CCPRestService.h"
#import "ASI/CLOPHTTPRequest.h"
#include "serphoneinterface.h"
#import <AVFoundation/AVFoundation.h>
#import "audio/YtxAQRecorder.h"
#import "Consumer.h"
#ifdef __cplusplus
extern "C" {
#endif
#import "amrFileCodec.h"
#ifdef __cplusplus
}
#endif

#import <CoreTelephony/CTCall.h>
#import <CoreTelephony/CTCallCenter.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import "CLOPcrypto.h"
static CCPCallService *g_CCPCallService;

@class UIStatusBarForegroundView;
@class UIStatusBarDataNetworkItemView;
@class StatisticsInfo;
enum{
    BarNetworkNOReachable = 0,
    BarNetwork2G          = 1,
    BarNetwork3G          = 2,
    BarNetworkWifi1       = 3,
    BarNetworkWifi2       = 5
};

enum ECapabilityResult{
    ECapability_Success = 0,    //获取能力集成功
    ECapability_Failed,         //获取能力集失败
    ECapability_Not,            //没有能力
    ECapability_Timeout         //获取能力集超时
};


/*************jiazy interphone**************/
enum ERunningType
{
    ERunningType_None = 0,
    ERunningType_Voip,
    ERunningType_Interphone,
    ERunningType_ChatRoom,
    ERunningType_VideoConference
};
/***********jiazy interphone end**************/

#define KEY_CCP_CALLTYPE        @"calltype"
#define KEY_CCP_CALLID          @"callid"
#define KEY_CCP_CALLER          @"caller"
#define KEY_CCP_CALLERDATA      @"callerData"
#define KEY_CCP_REASON          @"reason"
#define KEY_CCP_DESTIONATION    @"destionation"
#define KEY_CCP_DTMF            @"dtmf"
#define KEY_CCP_MSGID           @"msgid"
#define KEY_CCP_SENDER          @"sender"
#define KEY_CCP_RECEIVER        @"receiver"
#define KEY_CCP_MESSAGE         @"message"
#define KEY_CCP_MESSAGE_ID      @"message_id"
#define KEY_CCP_CALLED          @"called"
#define KEY_CCP_UPDATEMEDIA_REQUEST   @"updatemedia_request"
#define KEY_CCP_UPDATEMEDIA_RESPONSE  @"updatemedia_response"
#define KEY_CCP_EVENT_TYPE  @"eventType"
#define KEY_CCP_USERDATA    @"userdata"
#define KEY_CCP_INTTYPE         @"intType"
#define KEY_CCP_DATE            @"msg_datetime"
#define KEY_CCP_ROTATE            @"video_rotate"

#define KEY_CCP_AudioInData             @"audio_InData"
#define KEY_CCP_AudioOutData            @"audio_OutData"
#define KEY_CCP_AudioIsSendData         @"audio_IsSend_Data"

#define CCPSDK_BUNDLE_NAME @"CCPSDKBundle.bundle"
#define RING_NAME @"ring.wav"
#define RINGBACK_NAME @"ringback.wav"
#define RINGBUSY_NAME @"busy.wav"

/*
#define CAPABILITY_CALLING      @"calling"
#define CAPABILITY_MESSAGE      @"message"
#define CAPABILITY_CREATECONF   @"createconf"
#define CAPABILITY_JOINCONF     @"joinconf"
*/
#define RECORD_TIMEOUT          60
@interface CCPCallService() <AVAudioPlayerDelegate, AVAudioSessionDelegate,CCPRestDelegate,ConsumerSendChunkedDelegate>
{
    id<CCPCallEventDelegate> theDelegate;
    NSMutableDictionary *voipCallDict;
    boolean_t isRegisted;
    boolean_t isVoipMute;
    TReachability *hostReach;
    NetworkStatus NetStatus;
    NSMutableDictionary *recordSetting;
    BOOL isVoiceMsgPlaying;
    double lowPassResults;
    YtxAQRecorder*					recorder;
    NSMutableDictionary*       sendTask;//发送任务队列，用文件名作为key Data是一个消费者对象

}
@property (retain, nonatomic)CTTelephonyNetworkInfo *networkInfo;
@property (retain, nonatomic)AVAudioPlayer  *player;
@property (retain, nonatomic)NSString       *curRecordedFile;//当前录音文件名
@property (retain, nonatomic)NSString       *playRingName;      //来电铃声文件名
@property (retain, nonatomic)NSString       *playRingbackName;  //呼叫回铃声文件名
@property (retain, nonatomic)NSString       *playRingBusyName; //忙音文件名
@property (assign, nonatomic)BOOL           isCallAndRing;      //是否有电话和铃声中

@property (retain, nonatomic)CCPRestService *ccpRestService;

@property (retain, nonatomic)NSString *account;
@property (retain, nonatomic)NSString *password;

@property (retain, nonatomic)NSString* clpss;

@property (retain, nonatomic)NSString* ccp_server_ip;
@property (assign, nonatomic)NSInteger ccp_serverport;

@property (retain, nonatomic)NSString *userPhoneNumber;
@property (retain, nonatomic)NSString *userName;


@property (retain) VoipCall *registerVoipCall;
@property (retain, nonatomic)NSString *messageReceiver;
@property (retain, nonatomic)NSString *messageContent;

@property (retain, nonatomic)NSTimer *capabilityTimer;
@property (retain, nonatomic)NSTimer *recordTimer;
@property (retain, nonatomic)NSTimer *decibelsTimer;

/*************jiazy interphone**************/
@property (assign, nonatomic)ERunningType runningType; //现在运行的类型 用于实时语音 和 voip电话 之间的互斥
@property (retain, nonatomic)NSString *interphoneCallId;
@property (retain, nonatomic)NSString *strCapability;//base64编码的用户权限控制
/***********jiazy interphone end************/
@property (nonatomic, retain) CTCallCenter              *callCenter;

@property (nonatomic, retain) NSMutableArray            *longVoiceRoomNos;
@property (nonatomic, retain) NSMutableArray            *longVideoRoomNos;

@property (nonatomic, retain) NSString*           ccpValidate;

@property (nonatomic, retain) NSString*           lastTimeStr;

- (id)delegate;

//与云通讯平台连接成功
- (void)onConnected;
//与云通讯平台连接失败或连接断开
- (void)onConnectError:(NSNumber *)reason;
//有呼叫进入
- (void)onIncomingCallReceived:(NSDictionary *)dict;
//呼叫振铃
- (void)onCallAlerting:(NSString*)callid;
//所有应答
- (void)onCallAnswered:(NSString *)callid;
//外呼失败
- (void)onMakeCallFailed:(NSDictionary *)dict;
//本地Pause呼叫成功
- (void)onCallPaused:(NSString *)callid;
//呼叫被对端pasue
- (void)onCallPausedByRemote:(NSString *)callid;
//本地resume呼叫成功
- (void)onCallResumed:(NSString *)callid;
//呼叫被对端resume
- (void)onCallResumedByRemote:(NSString *)callid;
//呼叫挂机
- (void)onCallReleased:(NSString *)callid;
//呼叫被转接
- (void)onCallTransfered:(NSDictionary *)dict;

//收到文本短消息
- (void)onTextMessageReceived:(NSDictionary *)dict;

- (NSString *)createDigID;
- (VoipCall *)getVoipCallByDigID:(NSString *)digid;
- (void)reachabilityChanged:(NSNotification *)note;
- (void)checkReachabilty;

//播放来电铃声
- (void)playRing;
//播放呼叫回铃声
- (void)playRingback:(NSString *)callid;
//播放忙音
- (void)playRingBusy;
//停止铃声
- (void)stopRing:(BOOL)isCallOver;
//播放铃声
- (void)playSound:(NSString *)ringName;

- (NSString *)getResourceBundlePath:(NSString *)filename;

//如果进入通话流程，控制手机设置，如接近检测、不锁屏
- (void)enterVoipCallFlow:(BOOL)status;

- (NSNumber*)NetworkTypeFromStatusBar;
@end

static NSArray *const connectErrArr = [[NSArray alloc] initWithObjects:@"", @"网络不给力", @"鉴权失败", @"对方拒接", @"对方不在线", @"呼叫超时", @"对方忙", @"当前无网络", @"网络异常", @"账号在其他地方登陆", @"对方版本不支持音频", nil];

@implementation CCPCallService

@synthesize account;
@synthesize password;
@synthesize registerVoipCall;

@synthesize isCallAndRing;
@synthesize player;
@synthesize playRingName;
@synthesize playRingbackName;
@synthesize playRingBusyName;
@synthesize ccpRestService;
@synthesize userPhoneNumber;
@synthesize userName;

@synthesize messageReceiver;
@synthesize messageContent;
@synthesize curRecordedFile;

@synthesize capabilityTimer;
@synthesize recordTimer;
@synthesize decibelsTimer;
@synthesize callCenter;
/*************jiazy interphone**************/
@synthesize interphoneCallId;
@synthesize runningType;
/***********jiazy interphone end************/
@synthesize strCapability;

@synthesize longVoiceRoomNos;
@synthesize longVideoRoomNos;
@synthesize ccpValidate;

@synthesize lastTimeStr;

#pragma mark - CCPClient层回调函数
static void onGetCapabilityToken()
{
    /*
    if(g_CCPCallService
       && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onGetCapabilityToken)])
    {
        NSString *token = [[g_CCPCallService delegate] onGetCapabilityToken];
        setCapabilityToken([token cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    */
}

static void onConnected()
{
    if(g_CCPCallService)
    {
        [g_CCPCallService performSelectorOnMainThread:@selector(onConnected) withObject:nil waitUntilDone:NO];
        seRateAfterP2PSucceed(256);
    }
}

static void onDisconnected()
{
    if(g_CCPCallService)
    {
        [g_CCPCallService performSelectorOnMainThread:@selector(onDisconnected) withObject:nil waitUntilDone:NO];
    }
}

static void oneXosipThreadStop()
{
    if (g_CCPCallService) {
        [g_CCPCallService performSelectorOnMainThread:@selector(oneXosipThreadStop) withObject:nil waitUntilDone:NO];
    }
}

static void onConnectError(int reason)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSNumber *Reason = [NSNumber numberWithInt:reason];
        [g_CCPCallService performSelectorOnMainThread:@selector(onConnectError:) withObject:Reason waitUntilDone:NO];
    }
    [pool release];
}

static void onIncomingCallReceived(int callType, const char *callid, const char *caller)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        char buffer[256] = {'\0'};
        getUserData(USERDATA_FOR_INVITE, buffer, 256);
        NSString *callerData = [NSString stringWithCString:buffer encoding:NSUTF8StringEncoding];

        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSString *Caller = [NSString stringWithCString:caller encoding:NSUTF8StringEncoding];
        NSNumber *CallType = [NSNumber numberWithInt:callType];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys: CallType,KEY_CCP_CALLTYPE,CallID, KEY_CCP_CALLID, Caller, KEY_CCP_CALLER, callerData, KEY_CCP_CALLERDATA, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onIncomingCallReceived:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onCallProceeding(const char*callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallProceeding:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallAlerting(const char *callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallAlerting:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallAnswered(const char *callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallAnswered:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onMakeCallFailed(const char *callid,int reason)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSNumber *Reason = [NSNumber numberWithInt:reason];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, Reason, KEY_CCP_REASON, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onMakeCallFailed:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onCallPaused(const char* callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallPaused:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallPausedByRemote(const char *callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallPausedByRemote:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallResumed(const char* callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallResumed:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallResumedByRemote(const char *callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallResumedByRemote:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallReleased(const char *callid)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallReleased:) withObject:CallID waitUntilDone:NO];
    }
    [pool release];
}

static void onCallMediaUpdateRequest(const char*callid,int request)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSString *strReq = [NSString stringWithFormat:@"%d", (NSInteger)request];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, strReq, KEY_CCP_UPDATEMEDIA_REQUEST, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallMediaUpdateRequest:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onRecordVoiceStatus (const char *callid, const char *fileName, int status)
{

}

static void onAudioData(const char *callid, const void *inData, int inLen, void *outData, int &outLen, bool send)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onAudioDataWithCallid:andInData:andOutData:andOutLen:andIsSend:)])
        {
            NSString* strCallid = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
            NSData* in_Data = [NSData dataWithBytes:inData length:inLen];
            [[g_CCPCallService delegate] onAudioDataWithCallid:strCallid andInData:in_Data andOutData:outData andOutLen:&outLen andIsSend:send];
        }
    }
    [pool release];
}


static void onOriginalAudioData(const char *callid, const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onOriginalAudioDataWithCallid:andInData:andSampleRate:andNumChannels:andCodec:andIsSend:)])
        {
            NSString* strCallid = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
            NSData* in_Data = [NSData dataWithBytes:inData length:inLen];
            NSString* strcodec = [NSString stringWithCString:codec encoding:NSUTF8StringEncoding];
            [[g_CCPCallService delegate] onOriginalAudioDataWithCallid:strCallid andInData:in_Data andSampleRate:sampleRate andNumChannels:numChannels andCodec:strcodec andIsSend:send];
        }
    }
    [pool release];
}

static void onMessageRemoteVideoRotate(const char *degree)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *rotate = [NSString stringWithCString:degree encoding:NSUTF8StringEncoding];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:rotate, KEY_CCP_ROTATE, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onMessageRemoteVideoRotate:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onCallMediaUpdateResponse(const char*callid,int response)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSString *strRes= [NSString stringWithFormat:@"%d", (NSInteger)response];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, strRes, KEY_CCP_UPDATEMEDIA_RESPONSE, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallMediaUpdateResponse:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onCallTransfered(const char *callid , const char *destionation)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSString *Destionation = [NSString stringWithCString:destionation encoding:NSUTF8StringEncoding];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, Destionation, KEY_CCP_DESTIONATION, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onCallTransfered:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onDtmfReceived(const char *callid, char dtmf)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSString *Dtmf = [NSString stringWithFormat:@"%c", dtmf];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, Dtmf, KEY_CCP_DTMF, nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onDtmfReceived:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onMessageSendReport(const char*msgid, const char*time, int status)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *strmsgid= [NSString stringWithCString:msgid encoding:NSUTF8StringEncoding];
        NSString *strtime = nil;
        if (status == 100)
        {
           strtime = [NSString stringWithCString:time encoding:NSUTF8StringEncoding];
        }

        NSString *strstatus= [NSString stringWithFormat:@"%d", (NSInteger)status];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:strstatus, KEY_CCP_REASON, strmsgid, KEY_CCP_MSGID, strtime,KEY_CCP_DATE,nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onMessageSendReport:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onTextMessageReceived(const char *sender, const char *receiver, const char *sendtime, const char *msgid, const char *message, const char *userdata)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *Sender = [NSString stringWithCString:sender encoding:NSUTF8StringEncoding];
        NSString *Message = [NSString stringWithCString:message encoding:NSUTF8StringEncoding];

        NSString *dateTime = @"";
        if (sendtime != NULL)
        {
            NSString* date = [NSString stringWithCString:sendtime encoding:NSUTF8StringEncoding];
            if (date.length >= 14)
            {
                dateTime = [NSString stringWithFormat:@"%@-%@-%@ %@:%@:%@",
                            [date substringToIndex:4],
                            [date substringWithRange:NSMakeRange(4, 2)],
                            [date substringWithRange:NSMakeRange(6, 2)],
                            [date substringWithRange:NSMakeRange(8, 2)],
                            [date substringWithRange:NSMakeRange(10, 2)],
                            [date substringWithRange:NSMakeRange(12, 2)]];
            }
            else
            {
                dateTime = date;
            }
        }

        NSString *Receiver = @"";
        if (receiver != NULL)
        {
            Receiver = [NSString stringWithCString:receiver encoding:NSUTF8StringEncoding];
        }

        NSString *Userdata = @"";
        if (userdata != NULL)
        {
            Userdata = [NSString stringWithCString:userdata encoding:NSUTF8StringEncoding];
        }

        NSString *msgID = [NSString stringWithCString:msgid encoding:NSUTF8StringEncoding];

        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:Sender, KEY_CCP_SENDER, Message, KEY_CCP_MESSAGE,Receiver,KEY_CCP_RECEIVER,Userdata,KEY_CCP_USERDATA, dateTime, KEY_CCP_DATE,msgID,KEY_CCP_MESSAGE_ID,nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onTextMessageReceived:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onLogInfo(const char* loginfo)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *LoginInfo = [NSString stringWithCString:loginfo encoding:NSUTF8StringEncoding];
        [g_CCPCallService performSelectorOnMainThread:@selector(onLogInfo:) withObject:LoginInfo waitUntilDone:NO];
    }
    [pool release];
}

static void onNotifyGeneralEvent(const char*callid, int eventType, const char*userdata , int intdata)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        if (eventType == G_EVENT_EarlyMedia)
        {
            NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
            NSNumber *EventType = [NSNumber numberWithInt:eventType];
            NSString *UserData = [NSString stringWithCString:userdata encoding:NSUTF8StringEncoding];
            NSNumber *IntType = [NSNumber numberWithInt:intdata];
            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, EventType, KEY_CCP_EVENT_TYPE, UserData, KEY_CCP_USERDATA, IntType, KEY_CCP_INTTYPE, nil];
            [g_CCPCallService performSelectorOnMainThread:@selector(onNotifyGeneralEvent:) withObject:dict waitUntilDone:NO];
        }
        else if(eventType == G_EVENT_MessageCommand)
        {
            NSString *UserData = [NSString stringWithCString:userdata encoding:NSUTF8StringEncoding];
            [g_CCPCallService performSelectorOnMainThread:@selector(onPushVoiceMessage:) withObject:UserData waitUntilDone:NO];
        }
        else if(eventType == G_EVENT_RemoteVideoRatio)
        {
//            NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
//            NSString *UserData = [NSString stringWithCString:userdata encoding:NSUTF8StringEncoding];
//            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, UserData, KEY_CCP_USERDATA, nil];
//            [g_CCPCallService performSelectorOnMainThread:@selector(onCallVedioRatioChanged:) withObject:dict waitUntilDone:NO];
        }
        else if (eventType == G_EVENT_MediaInitFailed)
        {
            NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
            //NSNumber *EventType = [NSNumber numberWithInt:eventType];
            NSString *UserData = [NSString stringWithCString:userdata encoding:NSUTF8StringEncoding];
            NSNumber *IntType = [NSNumber numberWithInt:intdata];
            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, UserData, KEY_CCP_USERDATA, IntType, KEY_CCP_INTTYPE, nil];
            [g_CCPCallService performSelectorOnMainThread:@selector(onCallMediaInitFailed:) withObject:dict waitUntilDone:NO];
        }
        else if (eventType == G_EVENT_AudioDestinationChanged)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onFirewallPolicyEnabled)])
            {

                [[g_CCPCallService delegate] onFirewallPolicyEnabled];
            }
        }
        else if (eventType == G_EVENT_VideoDestinationChanged)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onFirewallPolicyEnabled)])
            {

                [[g_CCPCallService delegate] onFirewallPolicyEnabled];
            }
        }
    }
    [pool release];
}

static void onRemoteVideoRatioChanged(const char *callid, int width, int height, bool isVideoConference, const char *sipNo)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = @"";
        if (callid != NULL) {
            CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        }
        NSString *voip = @"";
        if (sipNo != NULL) {
            voip = [NSString stringWithCString:sipNo encoding:NSUTF8StringEncoding];
        }
        NSNumber *widthNum = [NSNumber numberWithInt:width];
        NSNumber *heightNum = [NSNumber numberWithInt:height];
        NSNumber *boolNum = [NSNumber numberWithBool:isVideoConference];

        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, voip, @"voip", widthNum, @"width", heightNum, @"height", boolNum, @"isConf", nil];

        [g_CCPCallService performSelectorOnMainThread:@selector(onCallVedioRatioChanged:) withObject:dict waitUntilDone:NO];

    }
    [pool release];
}

static void onRequestSpecifiedVideoFailed(const char *callid, const char *sip, int reason)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString *CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSNumber *Reason = [NSNumber numberWithInt:reason];
        NSString *caller = [NSString stringWithCString:sip encoding:NSUTF8StringEncoding];

        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, Reason, KEY_CCP_REASON, caller, KEY_CCP_CALLER,nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onRequestSpecifiedVideoFailed:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

static void onStopSpecifiedVideoResponse(const char *callid, const char *sip, int response, void *window)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if(g_CCPCallService)
    {
        NSString* CallID = [NSString stringWithCString:callid encoding:NSUTF8StringEncoding];
        NSString* voip = [NSString stringWithCString:sip encoding:NSUTF8StringEncoding];
        NSNumber *Reason = [NSNumber numberWithInt:response];
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, Reason, KEY_CCP_REASON, voip, KEY_CCP_CALLER,nil];
        [g_CCPCallService performSelectorOnMainThread:@selector(onStopSpecifiedVideoResponse:) withObject:dict waitUntilDone:NO];
    }
    [pool release];
}

#pragma mark -

- (void)onDisconnected
{
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onDisconnected)])
    {
        [[self delegate] onDisconnected];
    }
}

- (void)oneXosipThreadStop
{
    [self uninitServiceCore];
    [self reinitServiceCore];
//    connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
    if ([self.clpss length] > 0)
    {
        connectToCCPWithXML([self.clpss UTF8String],[self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    else
    {
        connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

//与云通讯平台连接成功
- (void)onConnected
{
    isRegisted = true;
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onConnected)])
    {
        [[self delegate] onConnected];
    }

    if(self.registerVoipCall)
    {
        const char *retCallid = makeCall(self.registerVoipCall.callType, [self.registerVoipCall.callee cStringUsingEncoding:NSUTF8StringEncoding]);
        if(retCallid)
        {
            NSString *callid = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];
            self.registerVoipCall.callID = callid;
            [voipCallDict setValue:self.registerVoipCall forKey:callid];
            self.registerVoipCall = nil;
            return;
        }
        else
        {
            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:self.registerVoipCall.digID, KEY_CCP_CALLID, [NSNumber numberWithInt:ELocalReason_MakeCallFailed], KEY_CCP_REASON, nil];
            [self performSelector:@selector(onMakeCallFailedPrivate:) withObject:dict afterDelay:0.1];
            self.registerVoipCall = nil;
            return;
        }
    }

//    NetworkStatus status = [hostReach currentReachabilityStatus];
//    if(status == NOReachable ||  status == NotReachable)
//    {
//        setNetworkType(NETWORK_STATUS_NONE, false, false);
//    }
//    else
//    {
//        if(status == ReachableViaWiFi)
//        {
//            setNetworkType(NETWORK_STATUS_WIFI, true, true);
//        }
//        else if(status == ReachableViaWWAN)
//        {
//            NSNumber* type  = [self NetworkTypeFromStatusBar];
//            if ([type intValue] == BarNetworkNOReachable) {
//                setNetworkType(NETWORK_STATUS_NONE, false, false);
//            }
//            else if([type intValue] == BarNetwork2G)
//            {
//                setNetworkType(NETWORK_STATUS_GPRS, true, true);
//            }
//            else if( [type intValue] == BarNetwork3G)
//            {
//                setNetworkType(NETWORK_STATUS_3G, true, true);
//            }
//            else if( [type intValue] == BarNetworkWifi1)
//            {
//                setNetworkType(NETWORK_STATUS_3G, true, true);
//            }
//            else if ([type intValue] == BarNetworkWifi2)
//            {
//                setNetworkType(NETWORK_STATUS_WIFI, true, true);
//            }
//        }
//    }
}

- (int)ios7NetworkType:(NSString*)radioAccessTechnology {
    if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyGPRS]) {
        return 0;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyEdge]) {
        return 0;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyWCDMA]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyHSDPA]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyHSUPA]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMA1x]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORev0]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORevA]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORevB]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyeHRPD]) {
        return 1;
    } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyLTE]) {
        return 2;
    }

    return 0;
}
//与云通讯平台连接失败或连接断开
- (void)onConnectError:(NSNumber *)reason
{
    isRegisted = false;
    if (!(self.account && self.password && self.clpss)) {
        return;
    }
    if (reason.integerValue == EReasonKickedOff)
    {
        if (runningType == ERunningType_Interphone)
        {
            [self exitInterphone];
        }
        else if(runningType == ERunningType_ChatRoom)
        {
            [self exitChatroom];
        }
        else if(runningType == ERunningType_VideoConference)
        {
            [self exitVideoConference];
        }
    }

    if (reason.integerValue == EReasonReFetchSoftSwitch)
    {
        [self.ccpRestService getSipAddress];
    }
    else
        if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onConnectError:withReasonMessge:)])
    {
        NSInteger intRet = reason.integerValue;
        NSString * msg = @"连接服务器失败";
        if (intRet>0 && intRet<connectErrArr.count) {
            msg = [connectErrArr objectAtIndex:intRet];
        }
        [[self delegate] onConnectError:intRet withReasonMessge:msg];
    }

    if(self.registerVoipCall)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] onConnectError register Failed reason=%d", [reason intValue]]];

        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:self.registerVoipCall.digID, KEY_CCP_CALLID, [NSNumber numberWithInt:ELocalReason_NotRegisted], KEY_CCP_REASON, nil];
        [self performSelector:@selector(onMakeCallFailedPrivate:) withObject:dict afterDelay:0.1];
        self.registerVoipCall = nil;
    }
}

//有呼叫进入
- (void)onIncomingCallReceived:(NSDictionary *)dict
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }
    [self enableLoudsSpeaker:YES];  //扬声器播放来电铃声

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onIncomingCallReceived:withCallerAccount:withCallerPhone:withCallerName:withCallType:)])
    {
        [self enterVoipCallFlow:TRUE];

        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
        NSString *Caller = [dict valueForKey:KEY_CCP_CALLER];
        NSString *CallerData = [dict valueForKey:KEY_CCP_CALLERDATA];
        NSNumber *CallType = [dict valueForKey:KEY_CCP_CALLTYPE];

        NSString *CallerPhone = nil;
        NSString *CallerName = nil;

        //字典加入一个新的voip记录
        NSArray *dataArray = [CallerData componentsSeparatedByString:@";"];
        for (NSString *data in dataArray) {
            NSString *tmpStr = @"tel=";
            NSRange range = [data rangeOfString:tmpStr];
            if(range.location != NSNotFound)
            {
                CallerPhone = [data substringWithRange:NSMakeRange(range.location+range.length, data.length-tmpStr.length)];
            }

            tmpStr = @"nickname=";
            range = [data rangeOfString:tmpStr];
            if(range.location != NSNotFound)
            {
                CallerName = [data substringWithRange:NSMakeRange(range.location+range.length, data.length-tmpStr.length)];
            }
        }

        NSString *DigID = [self createDigID];
        VoipCall *call = [[VoipCall alloc] init];
        call.digID = DigID;
        call.callID = CallID;
        call.callDirect = EIncoming;
        call.callType = [CallType intValue];
        call.callStatus = ECallRing;
        call.caller = Caller;
        [voipCallDict setValue:call forKey:CallID];
        [call release];
        [[self delegate] onIncomingCallReceived:DigID withCallerAccount:Caller withCallerPhone:CallerPhone withCallerName:CallerName withCallType:[CallType intValue]];
        if (!self.isCallAndRing)
        {
            [self playRing];
        }
    }
}

//呼叫服务器有响应
- (void)onCallProceeding:(NSString*)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallProceeding:)])
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallProceeding;
            [[self delegate] onCallProceeding:call.digID];
        }
    }
}

//呼叫振铃
- (void)onCallAlerting:(NSString*)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }
    [self enableLoudsSpeaker:NO];  //关闭扬声器,听筒播放来电铃声
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallAlerting:)])
    {
        if (!self.isCallAndRing)
        {
            [self onLogInfo:@"onCallAlerting startplay"];
            [self performSelector:@selector(playRingback:) withObject:callid afterDelay:2];
            //[self playRingback];
        }
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallAlerting;
            [[self delegate] onCallAlerting:call.digID];
        }
    }
}

//所有应答
- (void)onCallAnswered:(NSString *)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        CloopenReason *reason = [[CloopenReason alloc] init];
        reason.reason = ELocalReason_JoinFailed;
        reason.msg = @"加入失败";
        NSString *confNo = nil;
        if (self.interphoneCallId.length > 0)
        {
            VoipCall *interphoneCall = [voipCallDict valueForKey:self.interphoneCallId];
            if( interphoneCall != nil)
            {
                if ([callid isEqualToString:self.interphoneCallId])
                {
                    reason.reason = 0;
                    reason.msg = @"加入成功";
                    if ([interphoneCall.callee length] > 30)
                    {
                        confNo = [interphoneCall.callee substringToIndex:30];
                    }
                    else
                        confNo = interphoneCall.callee;
                    [self onLogInfo:@"startInterphone success"];
                    [self enableLoudsSpeaker:YES];
                }
                else
                {
                    [voipCallDict removeObjectForKey:self.interphoneCallId];
                }
            }
        }

        if (runningType == ERunningType_Interphone)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] ERunningType_Interphone onCallAnswered delegate"]];
                [[g_CCPCallService delegate] onInterphoneStateWithReason:reason andConfNo:confNo];
            }
        }
        else if (runningType == ERunningType_ChatRoom)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] ERunningType_ChatRoom onCallAnswered delegate"]];
                [[g_CCPCallService delegate] onChatroomStateWithReason:reason andRoomNo:confNo];
            }
        }

        else if (runningType == ERunningType_VideoConference)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] ERunningType_VideoConference onCallAnswered delegate"]];
                [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:confNo];
            }
        }

        if (reason.reason != 0)
        {
            runningType = ERunningType_None;
        }
        [reason release];
        return;
    }

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallAnswered:)])
    {
        [self stopRing:NO];

        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallStreaming;
            [[self delegate] onCallAnswered:call.digID];
        }
    }
}

//外呼失败
- (void)onMakeCallFailed:(NSDictionary *)dict
{
    NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
    NSNumber *Reason = [dict valueForKey:KEY_CCP_REASON];

    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] onMakeCallFailed Interphone Reason=%d",[Reason intValue]]];

        CloopenReason *reason = [[CloopenReason alloc] init];
        reason.reason = Reason.integerValue;
        VoipCall *interphoneCall = [voipCallDict valueForKey:self.interphoneCallId];

        NSString *confNo = nil;
        if (interphoneCall != nil)
        {
            if ([interphoneCall.callee length]>30)
            {
                confNo = [[NSString stringWithString:interphoneCall.callee] substringToIndex:30];
            }
            else
                confNo = interphoneCall.callee;
            [voipCallDict removeObjectForKey:self.interphoneCallId];
        }

        //调用回调函数
        if (runningType == ERunningType_Interphone)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] ERunningType_Interphone onMakeCallFailed delegate"]];
                [[g_CCPCallService delegate] onInterphoneStateWithReason:reason andConfNo:confNo];
            }
        }
        else if (runningType == ERunningType_ChatRoom)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] ERunningType_ChatRoom onMakeCallFailed delegate"]];
                [[g_CCPCallService delegate] onChatroomStateWithReason:reason andRoomNo:confNo];
            }
        }
        else if (runningType == ERunningType_VideoConference)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] ERunningType_VideoConference onMakeCallFailed delegate"]];
                [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:confNo];
            }
        }
        //重置状态
        runningType = ERunningType_None;

        [reason release];
        return;
    }

    VoipCall *call = [voipCallDict valueForKey:CallID];
    if(!call || call.callStatus == ECallEnd)
        return;

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] onMakeCallFailed CallID=%@ digID=%@ Reason=%d",CallID,call.digID,[Reason intValue]]];

    if(call.callStatus == ECallReleasing)
    {
        [self onCallReleased:call.callID];
        return;
    }
    else
    {
        NSString *tmpDigID = [NSString stringWithFormat:@"%@", call.digID];
        if([Reason intValue] == EReasonDeclined || [Reason intValue] == EReasonBusy )
        {
            [self enableLoudsSpeaker:NO];
            [self playRingBusy];
            call.callStatus = ECallFailedBusy;
        }
        else
        {
            [self enterVoipCallFlow:FALSE];
            [self enableLoudsSpeaker:NO];
            [self stopRing:YES];

            [voipCallDict removeObjectForKey:CallID];
        }
        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onMakeCallFailed:withReason:)])
        {
            NSInteger retReason = [Reason integerValue];
            [[self delegate] onMakeCallFailed:tmpDigID withReason:retReason];
        }
    }
}

//外呼失败
- (void)onMakeCallFailedPrivate:(NSDictionary *)dict
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }
    [self enterVoipCallFlow:FALSE];

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onMakeCallFailed:withReason:)])
    {
        NSString *digid = [dict valueForKey:KEY_CCP_CALLID];
        NSNumber *Reason = [dict valueForKey:KEY_CCP_REASON];
        NSInteger retReason = [Reason integerValue];
        [[self delegate] onMakeCallFailed:digid withReason:retReason];
    }
}

//本地Pause呼叫成功
- (void)onCallPaused:(NSString *)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallPaused:)])
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallPaused;
            [[self delegate] onCallPaused:call.digID];
        }
    }
}
//呼叫被对端pasuen
- (void)onCallPausedByRemote:(NSString *)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallPausedByRemote:)])
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallPausedByRemote;
            [[self delegate] onCallPausedByRemote:call.digID];
        }
    }
}

//本地resume呼叫成功
- (void)onCallResumed:(NSString *)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallResumed:)])
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallResumed;
            [[self delegate] onCallResumed:call.digID];
        }
    }
}

//呼叫被对端resume
- (void)onCallResumedByRemote:(NSString *)callid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallResumedByRemote:)])
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call)
        {
            call.callStatus = ECallResumedByRemote;
            [[self delegate] onCallResumedByRemote:call.digID];
        }
    }
}

//呼叫挂机
- (void)onCallReleased:(NSString *)callid
{
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] onCallReleased"]];
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
         || runningType == ERunningType_VideoConference)
    {
        const char * pCallid = getCurrentCall();
        if (pCallid == NULL)
        {
            VoipCall *call = [voipCallDict valueForKey:callid];
            if(call)
                [voipCallDict removeObjectForKey:callid];
            runningType = ERunningType_None;
            return;
        }
        NSString* currentCallid = [NSString stringWithUTF8String: pCallid];
        if ([currentCallid length] <=0) {
            return;
        }
        if(![callid isEqualToString: currentCallid])
        {
            VoipCall *call = [voipCallDict valueForKey:callid];
            if(call)
                [voipCallDict removeObjectForKey:callid];
        }
        else
            runningType = ERunningType_None;
        return;
    }

    runningType = ERunningType_None;
    [self setFirewallPolicy:phonePolicyUseIce];
    [self enterVoipCallFlow:FALSE];
    [self enableLoudsSpeaker:NO];
    [self stopRing:YES];

    VoipCall *call = [voipCallDict valueForKey:callid];
    if(!call)
        return;
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallReleased:)])
    {
        call.callStatus = (int)ECallEnd;
        [[self delegate] onCallReleased:call.digID];
    }
    if ([voipCallDict objectForKey:callid])
        [voipCallDict removeObjectForKey:callid];
}

- (void)onCallMediaUpdateRequest:(NSDictionary *)dict
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onSwitchCallMediaType:withRequest:)])
    {
        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
        NSString *request = [dict valueForKey:KEY_CCP_UPDATEMEDIA_REQUEST];

        VoipCall *call = [voipCallDict valueForKey:CallID];
        if(call)
        {
            [[self delegate] onSwitchCallMediaType:call.digID withRequest:request.integerValue];
        }
    }
}

- (void)onCallMediaUpdateResponse:(NSDictionary *)dict
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onSwitchCallMediaType:withResponse:)])
    {
        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
        NSString *response = [dict valueForKey:KEY_CCP_UPDATEMEDIA_RESPONSE];

        VoipCall *call = [voipCallDict valueForKey:CallID];
        if(call)
        {
            [[self delegate] onSwitchCallMediaType:call.digID withResponse:response.integerValue];
        }
    }
}

- (void)onMessageRemoteVideoRotate:(NSDictionary *)dict
{
    if ([self delegate] && [[self delegate] respondsToSelector:@selector(onMessageRemoteVideoRotate:)])
    {
        NSString *rotate = [dict valueForKey:KEY_CCP_ROTATE];
        [[self delegate] onMessageRemoteVideoRotate:rotate];
    }
}

//呼叫被转接
- (void)onCallTransfered:(NSDictionary *)dict
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return;
    }

    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallTransfered:transferTo:)])
    {
        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
        NSString *Destionation = [dict valueForKey:KEY_CCP_DESTIONATION];

        VoipCall *call = [voipCallDict valueForKey:CallID];
        if(call)
        {
            call.callStatus = ECallTransfered;
            [[self delegate] onCallTransfered:call.digID transferTo:Destionation];
        }
    }
}

-(void) onRequestSpecifiedVideoFailed:(NSDictionary *)dict
{
    if (self.delegate != nil && [self.delegate respondsToSelector:@selector(onRequestConferenceMemberVideoFailed:andConferenceId:andVoip:)])
    {
        NSString *conferenceId = nil;
        NSString *digid = [dict valueForKey:KEY_CCP_CALLID];

        NSNumber *Reason = [dict valueForKey:KEY_CCP_REASON];
        NSInteger retReason = [Reason integerValue];

        NSString *sip = [dict valueForKey:KEY_CCP_CALLER];

        VoipCall *call = [voipCallDict valueForKey:digid];
        if (call != nil)
        {
            conferenceId = call.callee;
        }

        CloopenReason *reason = [[CloopenReason alloc] init];
        reason.reason = retReason;
        [self.delegate onRequestConferenceMemberVideoFailed:reason andConferenceId:conferenceId andVoip:sip];
        [reason release];
    }
}

-(void)onStopSpecifiedVideoResponse:(NSDictionary *)dict
{
    if (self.delegate != nil && [self.delegate respondsToSelector:@selector(onCancelConferenceMemberVideo:andConferenceId:andVoip:)]) {
        NSString *conferenceId = nil;
        NSString *digid = [dict valueForKey:KEY_CCP_CALLID];

        NSNumber *Reason = [dict valueForKey:KEY_CCP_REASON];
        NSInteger response = [Reason integerValue];

        NSString *sip = [dict valueForKey:KEY_CCP_CALLER];
        VoipCall *call = [voipCallDict valueForKey:digid];
        if (call != nil)
        {
            conferenceId = call.callee;
        }

        CloopenReason *reason = [[CloopenReason alloc] init];
        reason.reason = response;
        [self.delegate onCancelConferenceMemberVideo:reason andConferenceId:conferenceId andVoip:sip];
        [reason release];
    }
}

//收到DTMF按键时的回调
- (void)onDtmfReceived:(NSDictionary *)dict
{
//    if (runningType == ERunningType_Interphone || runningType == ERunningType_ChatRoom)
//    {
//        return;
//    }
//
//    if([self delegate]
//       && [[self delegate] respondsToSelector:@selector(onDtmfReceived:dtmf:)])
//    {
//        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
//        NSString *Dtmf = [dict valueForKey:KEY_CCP_DTMF];
//
//        VoipCall *call = [voipCallDict valueForKey:CallID];
//        if(call)
//        {
//            call.callStatus = ECallTransfered;
//            [[self delegate] onDtmfReceived:call.digID dtmf:Dtmf];
//        }
//    }
}

- (void)onMessageSendReport:(NSDictionary *)dict
{
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onSendInstanceMessageWithReason:andMsg:)])
    {
        NSString *status = [dict valueForKey:KEY_CCP_REASON];
        NSString *Messageid = [dict valueForKey:KEY_CCP_MSGID];
        NSString *time = [dict valueForKey:KEY_CCP_DATE];
        NSInteger reason = [status integerValue];
        IMTextMsg * textmsg = [[IMTextMsg alloc] init];
        textmsg.dateCreated = time;
        if (reason == 200)
        {
            reason = 0;
            textmsg.status = @"1";
        }
        else if (reason == 100 || reason == 202 || reason == 404 || reason == 408 || reason == 477)
        {
            reason = 0;
            textmsg.status = @"0";
        }
        else
        {
            if (reason == -1)
            {
                NetworkStatus status = [hostReach currentReachabilityStatus];
                if (status == NotReachable)
                {
                    if(g_CCPCallService && [g_CCPCallService delegate]
                       && [[g_CCPCallService delegate] respondsToSelector:@selector(onReachbilityChanged:)])
                    {
                        [[g_CCPCallService delegate] onReachbilityChanged:NETWORK_STATUS_NONE];
                    }
                }
                else if(status == ReachableViaWiFi)
                {
                    setNetworkType(NETWORK_STATUS_WIFI, true, true);
                }
                else if(status == ReachableViaWWAN)
                {
                    [self performSelector:@selector(reachabilityChangedWLAN:) withObject:nil afterDelay:0.5];
                }
            }
            textmsg.status = @"-1";
        }

        textmsg.msgId = Messageid;
        CloopenReason *clpresaon = [[CloopenReason alloc] init];
        clpresaon.reason = reason;
        [[self delegate] onSendInstanceMessageWithReason:clpresaon andMsg:textmsg];
        [clpresaon release];
        [textmsg release];
    }
}

//收到文本短消息
- (void)onTextMessageReceived:(NSDictionary *)dict
{
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onReceiveInstanceMessage:)])
    {
        IMTextMsg * textmsg = [[IMTextMsg alloc] init];
        NSString *Sender = [dict valueForKey:KEY_CCP_SENDER];
        NSString *Message = [dict valueForKey:KEY_CCP_MESSAGE];
        NSString *Receiver = [dict valueForKey:KEY_CCP_RECEIVER];
        NSString *Userdata = [dict valueForKey:KEY_CCP_USERDATA];
        textmsg.msgId = [dict valueForKey:KEY_CCP_MESSAGE_ID];
        textmsg.receiver = Receiver;
        textmsg.sender = Sender;
        textmsg.message = Message;
        textmsg.userData = Userdata;
        textmsg.dateCreated = [dict valueForKey:KEY_CCP_DATE];
        [[self delegate] onReceiveInstanceMessage:textmsg];
        [textmsg release];
    }
}

//收到视频通话，对方视频分辨率改变
-(void)onCallVedioRatioChanged:(NSDictionary *) dict
{
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallVideoRatioChanged:andVoIP:andIsConfrence:andWidth:andHeight:)])
    {
//        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:CallID, KEY_CCP_CALLID, voip, @"voip", widthNum, @"width", heightNum, @"height", boolNum, @"isConf", nil];

        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
        NSString *voip = [dict valueForKey:@"voip"];
        NSNumber *widthNum = [dict valueForKey:@"width"];
        NSNumber *heightNum = [dict valueForKey:@"height"];
        NSNumber *boolNum = [dict valueForKey:@"isConf"];

        [[self delegate] onCallVideoRatioChanged:CallID andVoIP:voip andIsConfrence:boolNum.boolValue andWidth:widthNum.integerValue andHeight:heightNum.integerValue];
    }
}

//呼叫时，媒体初始化失败
-(void)onCallMediaInitFailed:(NSDictionary *) dict
{
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onCallMediaInitFailed:withMediaType:withReason:)])
    {
        NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
        //NSNumber *EventType = [dict valueForKey:KEY_CCP_EVENT_TYPE];
        NSString *UserData = [dict valueForKey:KEY_CCP_USERDATA];
        NSNumber *IntType =  [dict valueForKey:KEY_CCP_INTTYPE];
        [[self delegate] onCallMediaInitFailed:CallID  withMediaType:UserData.integerValue withReason:IntType.integerValue];
    }
}


//收到sip消息
-(void)onPushVoiceMessage:(NSString *) xmlStr
{
    [self onLogInfo:[NSString stringWithFormat:@"[onPushVoiceMessage] message=%@", xmlStr]];
    NSMutableDictionary *messageDic = [[NSMutableDictionary alloc] init];
    [CCPRestService parseVoiceMessage:xmlStr andVoiceInfo:messageDic andVoipId:self.account];

    NSString *msgType = [messageDic objectForKey:KEY_VOICEMESSAGE_TYPE];
    NSInteger type = msgType.length>0?msgType.integerValue:0;
    if (type == 101)
    {

    }
    else if(type >= 201 && type <= 206 )
    {
        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onReceiveInterphoneMsg:)])
        {
            [self.delegate onReceiveInterphoneMsg:[messageDic objectForKey:KEY_VOICEMESSAGE_DATA]];
        }
    }
    else if (type >= 301 && type <= 304)
    {
        BOOL isSelfKickOff = NO;//自己被踢
        if (type == 304)
        {
            ChatroomMsg* msg = [messageDic objectForKey:KEY_VOICEMESSAGE_DATA];
           if([msg isKindOfClass:[ChatroomRemoveMemberMsg class]])
            {
                if ([((ChatroomRemoveMemberMsg*)msg).who isEqualToString:self.account])//自己被踢出
                    isSelfKickOff = YES;
            }
        }
        if (type==303 || isSelfKickOff)
        {
            [self enableLoudsSpeaker:NO];
            if (runningType == ERunningType_ChatRoom)
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] releaseCall exitChatRoom\r\n"]];
                VoipCall* call = [voipCallDict valueForKey:self.interphoneCallId];
                runningType = ERunningType_None;
                if (call)
                {
                    releaseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], 0);
                    if (type == 303)
                    {
                        for (NSString *str in self.longVoiceRoomNos)
                        {
                            if ([str isEqualToString:call.callee])
                            {
                                [self.longVoiceRoomNos removeObject:str];
                                break;
                            }
                        }
                    }
                    [voipCallDict removeObjectForKey:call.callID];
                }
            }
        }
        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onReceiveChatroomMsg:)])
        {
            [self.delegate onReceiveChatroomMsg:[messageDic objectForKey:KEY_VOICEMESSAGE_DATA]];
        }
    }
    else if((type >= 401 && type <= 408) || type == 501 || type == 901)
    {
        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onReceiveInstanceMessage:)])
        {
            [self.delegate onReceiveInstanceMessage:[messageDic objectForKey:KEY_VOICEMESSAGE_DATA]];
        }
    }
    else if (type >= 601 && type <= 608)
    {
        BOOL isSelfKickOff = NO;//自己被踢
        if (type == 604)
        {
            VideoConferenceMsg* msg = [messageDic objectForKey:KEY_VOICEMESSAGE_DATA];
            if([msg isKindOfClass:[VideoConferenceMsg class]])
            {
                if ([((VideoConferenceRemoveMemberMsg*)msg).who isEqualToString:self.account])//自己被踢出
                    isSelfKickOff = YES;
            }
        }
        if (type==603 || isSelfKickOff)
        {
            [self enableLoudsSpeaker:NO];
            if (runningType == ERunningType_VideoConference)
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] releaseCall exitVideoConference\r\n"]];
                VoipCall* call = [voipCallDict valueForKey:self.interphoneCallId];
                runningType = ERunningType_None;
                if (call)
                {
                    releaseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], 0);
                    if (type == 603)
                    {
                        for (NSString *str in self.longVideoRoomNos)
                        {
                            if ([str isEqualToString:call.callee])
                            {
                                [self.longVideoRoomNos removeObject:str];
                                break;
                            }
                        }
                    }
                    [voipCallDict removeObjectForKey:call.callID];
                }
            }
        }
        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onReceiveVideoConferenceMsg:)])
        {
            [self.delegate onReceiveVideoConferenceMsg:[messageDic objectForKey:KEY_VOICEMESSAGE_DATA]];
        }
    }
    [messageDic release];
}

//收到通知事件
- (void)onNotifyGeneralEvent:(NSDictionary *)dict
{
    NSString *CallID = [dict valueForKey:KEY_CCP_CALLID];
    NSNumber *EventType = [dict valueForKey:KEY_CCP_EVENT_TYPE];
//    NSString *UserData = [dict valueForKey:KEY_CCP_USERDATA];
//    NSNumber *IntType =  [dict valueForKey:KEY_CCP_INTTYPE];

    switch(EventType.intValue)
	{
        case G_EVENT_EarlyMedia:
        {
            VoipCall *call = [voipCallDict valueForKey:CallID];
            if(call)
            {
                [self enableLoudsSpeaker:NO];  //关闭扬声器
                [self stopRing:NO];
                call.callStatus = ECallEarlyMedia;
                [self onCallAlerting:call.digID];
            }
        }
            break;
        default:
            break;
    }
}

-(void) onReadyToSendChunkedMsgWithFileName:(NSString*) fileName andReceiver:(NSString*) receiver andChunked:(BOOL) chunked andUserdata:(NSString*)userdata andSendMsgId:(NSString*)msgid;
{
    [self.ccpRestService sendMediaMsgWithFileName:fileName andReceiver:receiver andChunked:chunked andUserdata:userdata andSendMsgId:msgid];
}


-(void) onsendChunkedWithReason:(NSInteger)reason andInstanceMsg:(InstanceMsg*) data
{
    IMAttachedMsg* msg = (IMAttachedMsg*)data;
    if (msg)
    {
        [sendTask removeObjectForKey:msg.fileUrl];
    }
    switch (reason)
    {
        case ELocalReason_UploadConnectFailed:
            return;
            break;

        default:
            break;
    }
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onSendInstanceMessageWithReason:andMsg:)])
    {
        CloopenReason *creason = [[CloopenReason alloc] init];
        creason.reason = reason;
        [[g_CCPCallService delegate] onSendInstanceMessageWithReason:creason andMsg:data];
        [creason release];
    }
}
//底层日志输出
- (void)onLogInfo:(NSString *)loginfo
{
//    NSLog(@"debug-onLogInfo %@",loginfo);
    if([self delegate]
       && [[self delegate] respondsToSelector:@selector(onLogInfo:)])
    {
        [[self delegate] onLogInfo:loginfo];
    }
}

#pragma mark - 初始化处理函数
static CCPCallService * ccpcallserviceSharedInstance;

+ (CCPCallService *)sharedInstance
{
    static dispatch_once_t  ccpcallserviceonceToken;
    dispatch_once(&ccpcallserviceonceToken, ^{
        ccpcallserviceSharedInstance = [[super allocWithZone:nil] initSDKCallBack];
    });
    return ccpcallserviceSharedInstance;
}

+ (id)new
{
    NSAssert(NO, @"CCPCallService: use +sharedInstance");
    return nil;
}

- (id)init
{
    return [self initSDKCallBack];
}

- (id)initSDKCallBack
{
//    return [self initWithDelegate:nil];
//}
//
//- (CCPCallService *)initWithDelegate:(id)delegate
//{
    self = [super init];
    if (self)
    {
        isRegisted = false;
        g_CCPCallService = self;
        static CCallbackInterface  interface;
        memset(&interface, 0, sizeof(CCallbackInterface));
        interface.onGetCapabilityToken = onGetCapabilityToken;
        interface.onConnected = onConnected;
        interface.onConnectError = onConnectError;
        interface.onIncomingCallReceived = onIncomingCallReceived;
        interface.onCallProceeding = onCallProceeding;
        interface.onCallAlerting = onCallAlerting;
        interface.onCallAnswered = onCallAnswered;
        interface.onMakeCallFailed = onMakeCallFailed;
        interface.onCallPaused = onCallPaused;
        interface.onCallPausedByRemote = onCallPausedByRemote;
        interface.onCallReleased = onCallReleased;
        interface.onCallTransfered = onCallTransfered;
        interface.onDtmfReceived = onDtmfReceived;
        interface.onTextMessageReceived = onTextMessageReceived;
        interface.onMessageSendReport = onMessageSendReport;
        interface.onLogInfo = onLogInfo;
        interface.onNotifyGeneralEvent = onNotifyGeneralEvent;
        interface.onCallMediaUpdateRequest = onCallMediaUpdateRequest;
        interface.onCallMediaUpdateResponse = onCallMediaUpdateResponse;
        interface.onRecordVoiceStatus = onRecordVoiceStatus;
        interface.onMessageRemoteVideoRotate = onMessageRemoteVideoRotate;
        interface.onRequestSpecifiedVideoFailed = onRequestSpecifiedVideoFailed;
        interface.onAudioData = onAudioData;
        interface.onOriginalAudioData = onOriginalAudioData;
        interface.onStopSpecifiedVideoResponse = onStopSpecifiedVideoResponse;
        interface.onRemoteVideoRatioChanged = onRemoteVideoRatioChanged;
        interface.onLogOut = onDisconnected;
        interface.oneXosipThreadStop = oneXosipThreadStop;

        initialize(&interface);
        voipCallDict = [[NSMutableDictionary alloc] init];
//        setCodecEnabled(4, 0);
        setDtxEnabled(true);
//        setTraceFlag(true);
        self.player = nil;
        recorder = new YtxAQRecorder();
        self.playRingName = nil;
        self.playRingbackName = nil;
        self.playRingBusyName = nil;
        self.isCallAndRing = FALSE;
        self.account = nil;
        self.password = nil;
        self.registerVoipCall = nil;
        self.playRingName = [self getResourceBundlePath:RING_NAME];
        self.playRingbackName = [self getResourceBundlePath:RINGBACK_NAME];
        self.playRingBusyName = [self getResourceBundlePath:RINGBUSY_NAME];
        self.isCallAndRing = NO;
        self.userPhoneNumber = nil;
        self.userName = nil;
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appDidEnterBackgroundFun:) name:UIApplicationDidEnterBackgroundNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appWillEnterForegroundFun:) name:UIApplicationWillEnterForegroundNotification object:nil];

        [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
        [[ NSNotificationCenter defaultCenter ] addObserver:self selector:@selector(checkBattery)name:UIDeviceBatteryStateDidChangeNotification object:nil];
        [[ NSNotificationCenter defaultCenter ] addObserver:self selector:@selector(checkBattery)name:UIDeviceBatteryLevelDidChangeNotification object:nil];

        //rest
        CCPRestService *tempRestService = [[CCPRestService alloc] init];
        self.ccpRestService = tempRestService;
        self.ccpRestService.chunkedDelegate = self;
        self.ccpRestService.libSDK = [self getLIBVersion];
        NSString* strDeviceVersion = [self getDeviceVersion];
        self.ccpRestService.deviceVersion = strDeviceVersion;
        [tempRestService release];

        if ([strDeviceVersion isEqualToString:@"iPhone 1G"]||[strDeviceVersion isEqualToString:@"iPhone 3G"]||[strDeviceVersion isEqualToString:@"iPhone 3GS"]||[strDeviceVersion isEqualToString:@"iPhone 4"]||[strDeviceVersion isEqualToString:@"iPod Touch 1G"]||[strDeviceVersion isEqualToString:@"iPod Touch 2G"]||[strDeviceVersion isEqualToString:@"iPod Touch 3G"]||[strDeviceVersion isEqualToString:@"iPod Touch 4G"]||[strDeviceVersion isEqualToString:@"iPad"]||[strDeviceVersion isEqualToString:@"Verizon iPhone 4"])
        {
            [self setRateAfterP2PSucceed:150];
        }

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getSipAddressFinished:) name:kGetSipAddressFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getNetGroupIdFinished:) name:kGetNetGroupIdFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getCallbackFinished:) name:kGetCallbackFinished object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getRestLoginfo:) name:kGetRestLoginfo object:nil];

        //intercom rest api
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(startinterphoneFinished:) name:kStartInterphoneFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controlMicFinished:) name:kControlMicFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(releaseMicFinished:) name:kReleaseMicFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getInterphoneMemberListFinished:) name:kGetInterphoneMemberFinished object:nil];

        //chatroom rest api
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(startChatroomHttpFinished:) name:kStartChatroomFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(inviteMembersToChatroomFinished:) name:kInviteMembersChatroomFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getChatroomslistFinished:) name:kGetChatRoomsListFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getChatRoomMemberListFinished:) name:kGetChatRoomMemberFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(dismissChatRoomFinished:) name:kDismissChatRoomFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(removeMemberChatRoomFinished:) name:kRemoveMemberFromChatRoomFinished object:nil];

        //mediaMsg rest api
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(sendMediaMsgFinished:) name:kSendMediaMsgFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(uploadMediaMsgFinished:) name:kUploadMediaMsgFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(downloadMediaMsgsFinished:) name:kDownloadMediaMsgFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(confirmDownloadAttachedFinished:) name:kConfirmDownloadAttachFinished object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(checkVoipOfflineMsgFinished:) name:kCheckVoipOfflineMsgFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getVoipOfflineMsgFinished:) name:kGetVoipOfflineMsgFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(confirmOfflineMsgFinished:) name:kConfirmOfflineMsgFinished object:nil];

        //视频会议 rest api
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(startVideoConferenceHttpFinished:) name:kStartVideoConferenceFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getVideoConferenceslistFinished:) name:kGetVideoConferenceListFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getVideoConferenceMemberListFinished:) name:kGetVideoConferenceMemberFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(dismissVideoConferenceFinished:) name:kDismissVideoConferenceFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(removeMemberVideoConferenceFinished:) name:kRemoveMemberFromVideoConferenceFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(switchRealScreenFinished:) name:kSwitchRealScreenFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(sendVideoConferencePortraitMsgFinished:) name:kSendVideoConferencePortraitFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getVideoConferencePortraitsMsgFinished:) name:kGetVideoConferencePortraitFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(downloadVideoPortraitMsgsFinished:) name:kDownloadVideoConferencePortraitFinished object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(startMultiVideoHttpFinished:) name:kStartMultiVideoFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(publishVideoInMultiVideoConference:) name:kPublishVideoMultiVideoConferenceFinished object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(unpublishVideoInMultiVideoConference:) name:kUnpublishVideoMultiVideoConferenceFinished object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(editTestNumFinished:) name:kEditTestNumFinished object:nil];

    }

    AVAudioSession *avsession = [AVAudioSession sharedInstance];
    avsession.delegate  = self;

    /*************jiazy interphone**************/
    runningType = ERunningType_None;
    /***********jiazy interphone end************/

    theDelegate = nil;
    sendTask = [[NSMutableDictionary alloc] init];
    [self handleCallEvent];

    self.longVoiceRoomNos = nil;
    self.longVideoRoomNos = nil;
    [self checkReachabilty];
    self.networkInfo = [[CTTelephonyNetworkInfo alloc] init];
    return self;
}


- (void)reinitServiceCore
{
    static CCallbackInterface  interface;
    memset(&interface, 0, sizeof(CCallbackInterface));
    interface.onGetCapabilityToken = onGetCapabilityToken;
    interface.onConnected = onConnected;
    interface.onConnectError = onConnectError;
    interface.onIncomingCallReceived = onIncomingCallReceived;
    interface.onCallProceeding = onCallProceeding;
    interface.onCallAlerting = onCallAlerting;
    interface.onCallAnswered = onCallAnswered;
    interface.onMakeCallFailed = onMakeCallFailed;
    interface.onCallPaused = onCallPaused;
    interface.onCallPausedByRemote = onCallPausedByRemote;
    interface.onCallReleased = onCallReleased;
    interface.onCallTransfered = onCallTransfered;
    interface.onDtmfReceived = onDtmfReceived;
    interface.onTextMessageReceived = onTextMessageReceived;
    interface.onMessageSendReport = onMessageSendReport;
    interface.onLogInfo = onLogInfo;
    interface.onNotifyGeneralEvent = onNotifyGeneralEvent;
    interface.onCallMediaUpdateRequest = onCallMediaUpdateRequest;
    interface.onCallMediaUpdateResponse = onCallMediaUpdateResponse;
    interface.onRecordVoiceStatus = onRecordVoiceStatus;
    interface.onMessageRemoteVideoRotate = onMessageRemoteVideoRotate;
    interface.onRequestSpecifiedVideoFailed = onRequestSpecifiedVideoFailed;
    interface.onAudioData = onAudioData;
    interface.onOriginalAudioData = onOriginalAudioData;
    interface.onStopSpecifiedVideoResponse = onStopSpecifiedVideoResponse;
    interface.onRemoteVideoRatioChanged = onRemoteVideoRatioChanged;
    interface.onLogOut = onDisconnected;
    interface.oneXosipThreadStop = oneXosipThreadStop;

    initialize(&interface);
    setCodecEnabled(4, 0);
    setDtxEnabled(true);
    setTraceFlag(true);
}

- (void)uninitServiceCore
{
    unInitialize();
}

- (id)delegate
{
    return theDelegate;
}

- (void)setDelegate:(id)delegate
{
    theDelegate = delegate;
}

- (void)dealloc
{
    [[CLOPHTTPRequest sharedQueue].operations makeObjectsPerformSelector:@selector(clearDelegatesAndCancel)];
    theDelegate = nil;
    g_CCPCallService = nil;
    disConnectToCCP();
    unInitialize();
    [voipCallDict release];
    self.callCenter.callEventHandler = nil;
    self.callCenter = nil;

    theDelegate = nil;
    self.ccpRestService.chunkedDelegate = nil;

    self.player = nil;
    delete recorder;
    //self.recorder = nil;
    self.playRingName = nil;
    self.playRingbackName = nil;
    self.playRingBusyName = nil;
    self.interphoneCallId = nil;

    self.account = nil;
    self.password = nil;
    self.registerVoipCall = nil;

    self.userPhoneNumber = nil;
    self.userName = nil;

    self.messageReceiver = nil;
    self.messageContent = nil;
    self.curRecordedFile = nil;
    self.strCapability = nil;
    if (hostReach != nil)
    {
        [hostReach stopNotifier];
        [hostReach release];
        hostReach = nil;
    }

    if(self.capabilityTimer)
    {
        [self.capabilityTimer invalidate];
        self.capabilityTimer = nil;
    }

    if (self.recordTimer)
    {
        [self.recordTimer invalidate];
        self.recordTimer = nil;
    }

    if (self.decibelsTimer)
    {
        [self.decibelsTimer invalidate];
        self.decibelsTimer = nil;
    }


    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationWillEnterForegroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationDidEnterBackgroundNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceBatteryStateDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIDeviceBatteryLevelDidChangeNotification object:nil];
//    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kReachabilityChangedNotification object:nil];
    self.ccpRestService = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetSipAddressFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetNetGroupIdFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetCallbackFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetRestLoginfo object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kStartInterphoneFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kControlMicFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kReleaseMicFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetInterphoneMemberFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kStartChatroomFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kInviteMembersChatroomFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetChatRoomsListFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetChatRoomMemberFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kDismissChatRoomFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kRemoveMemberFromChatRoomFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kSendMediaMsgFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kUploadMediaMsgFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kDownloadMediaMsgFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kConfirmDownloadAttachFinished object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self name:kCheckVoipOfflineMsgFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetVoipOfflineMsgFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kConfirmOfflineMsgFinished object:nil];

    //视频会议
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kStartVideoConferenceFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetVideoConferenceListFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetVideoConferenceMemberFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kDismissVideoConferenceFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kRemoveMemberFromVideoConferenceFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kSwitchRealScreenFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kSendVideoConferencePortraitFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kGetVideoConferencePortraitFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kDownloadVideoConferencePortraitFinished object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self name:kStartMultiVideoFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kPublishVideoMultiVideoConferenceFinished object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:kUnpublishVideoMultiVideoConferenceFinished object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self name:kEditTestNumFinished object:nil];

    [sendTask release];
    self.longVoiceRoomNos = nil;
    self.longVideoRoomNos = nil;
    self.clpss = nil;
    self.ccp_server_ip = nil;
    self.ccpValidate = nil;
    self.networkInfo = nil;
    self.lastTimeStr = nil;
    [super dealloc];
}

#pragma mark - 电话的基本处理函数
- (NSInteger)connectToCCP:(NSString *)proxy_addr onPort:(NSInteger)proxy_port withAccount:(NSString *)accountStr withPsw:(NSString *)passwordStr withAccountSid:(NSString *)accountSid withAuthToken:(NSString *)authToken
{
    if(proxy_addr.length == 0  || accountStr.length == 0  || passwordStr.length == 0)
        return ECCP_Failed;

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] connectToCCP restip=%@ restport=%d account=%@ password=%@ accountSid=%@ authToken=%@",proxy_addr,(int)proxy_port,accountStr,passwordStr,accountSid,authToken]];

    if ([passwordStr isEqualToString:@"1234"]) {
            self.ccp_server_ip = @"192.168.178.138";
            self.ccp_serverport = 7600;

            self.account = accountStr;
            self.password = passwordStr;
            connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    else
    {
        self.account = accountStr;
        self.password = passwordStr;
        self.ccpRestService.restip = proxy_addr;
        self.ccpRestService.restport = proxy_port;
        self.ccpRestService.account = accountStr;
        self.ccpRestService.password = passwordStr;
        self.ccpRestService.accountSid = accountSid;
        self.ccpRestService.authToken = authToken;
        if (self.ccpRestService && [self.password length]>0 && [self.ccpValidate length]>0)
        {
            NSString *xmlBody = [NSString stringWithFormat:@"<Switch><validate>%@</validate><voippwd>%@</voippwd></Switch>",self.ccpValidate,self.password];
            NSString *strxmlBody = @"";
            if ([xmlBody length] >0)
            {
                strxmlBody = [CLOPEncrypt clop_encodedData:xmlBody andPrivateKey:@"06dc87af5f37a004da50ceeb32a1b9c7"];
                ccpRestService.ValidateStr = strxmlBody;
            }
        }
        [self.ccpRestService getSipAddress];
    }



    return ECCP_Success;
}

- (void)setSelfPhoneNumber:(NSString *)phoneNumber
{
    self.userPhoneNumber = [NSString stringWithFormat:@"%@", phoneNumber];
}

- (void)setSelfName:(NSString *)nickName
{
    self.userName = [NSString stringWithFormat:@"%@", nickName];
}

- (NSString *)makeCallWithType:(NSInteger)callType andCalled:(NSString *)called;
{
    NSString* strVersion = [self getLIBVersion];
    NSArray* arr = [strVersion componentsSeparatedByString:@"#"];
    BOOL voice = [[arr objectAtIndex:3] isEqualToString:@"voice=true"];
    BOOL video = [[arr objectAtIndex:4] isEqualToString:@"video=true"];
    if ((!voice)&&(!video))
    {
        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onMakeCallFailed:withReason:)])
        {
            [[self delegate] onMakeCallFailed:nil withReason:ELocalReason_SDKNotSupport];
        }
        return nil;
    }
    if (callType == 1)//视频
    {
        if (!video)
        {
            if([self delegate]
               && [[self delegate] respondsToSelector:@selector(onMakeCallFailed:withReason:)])
            {
                [[self delegate] onMakeCallFailed:nil withReason:ELocalReason_SDKNotSupport];
            }
            return nil;
        }
    }
    else//音频
    {
        if (!voice)
        {
            if([self delegate]
               && [[self delegate] respondsToSelector:@selector(onMakeCallFailed:withReason:)])
            {
                [[self delegate] onMakeCallFailed:nil withReason:ELocalReason_SDKNotSupport];
            }
            return nil;
        }
    }

    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] makeCall Failed Of running else voip"]];

        if([self delegate]
           && [[self delegate] respondsToSelector:@selector(onMakeCallFailed:withReason:)])
        {
            [[self delegate] onMakeCallFailed:nil withReason:ELocalReason_CallBusy];
        }
        return nil;
    }

    NSString *DigID = [self createDigID];

    NSString *phone = self.userPhoneNumber.length>0?self.userPhoneNumber:@"";
    NSString *name = self.userName.length>0?self.userName:@"";
    NSString *userdata = [NSString stringWithFormat:@"tel=%@;nickname=%@;", phone, name];
    setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);

    [self enterVoipCallFlow:TRUE];

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] makeCall isRegisted=%d called=%@ callType=%d\r\n", isRegisted,called,callType]];
    if (!isRegisted)
    {

        if(self.account && self.password && (self.clpss || (self.ccp_server_ip && self.ccp_serverport)))
        {
            if ([self.clpss length] > 0)
            {
                connectToCCPWithXML([self.clpss UTF8String],[self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
            }
            else
            {
                connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
            }

            VoipCall *call = [[VoipCall alloc] init];
            call.digID = DigID;
            call.callDirect = EOutgoing;
            call.callType = callType;
            call.callStatus = ECallOutgoing;
            call.callee = called;
            self.registerVoipCall = call;
            [call release];
        }
        else
        {
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] makeCall no Account Failed"]];

            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:DigID, KEY_CCP_CALLID, [NSNumber numberWithInt:ELocalReason_NotRegisted], KEY_CCP_REASON, nil];
            [self performSelector:@selector(onMakeCallFailedPrivate:) withObject:dict afterDelay:0.1];
            return DigID;
        }
    }
    if ([called length] > 4)
    {
        if ([[called substringToIndex:4] isEqualToString:@"0086"] || [[called substringToIndex:3] isEqualToString:@"+86"])
        {
            setFirewallPolicy(SerphonePolicyNoFirewall);
        }
        else
            setFirewallPolicy(SerphonePolicyUseIce);
    }
    const char *retCallid = makeCall(callType,[called cStringUsingEncoding:NSUTF8StringEncoding]);

    if(retCallid)
    {
        runningType = ERunningType_Voip;

        NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];

        VoipCall *call = [[VoipCall alloc] init];
        call.digID = DigID;
        call.callID = CallID;
        call.callDirect = EOutgoing;
        call.callType = callType;
        call.callStatus = ECallOutgoing;
        call.callee = called;
        [voipCallDict setValue:call forKey:CallID];
        [call release];
        return DigID;
    }
    else
    {
        NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:DigID, KEY_CCP_CALLID, [NSNumber numberWithInt:ELocalReason_MakeCallFailed], KEY_CCP_REASON, nil];
        [self performSelector:@selector(onMakeCallFailedPrivate:) withObject:dict afterDelay:0.1];
        return DigID;
    }
}

- (NSInteger)makeCallback:(NSString *)src withTOCall:(NSString *)dest andSrcSerNum:(NSString*) srcSerNum andDestSerNum:(NSString*) destSerNum;
{
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] callback fromPhone=%@ toPhone=%@\r\n", src,dest]];
    if(!src || !dest)
        return ECCP_Failed;

    [self.ccpRestService getCallback:src withTOCall:dest andSrcSerNum:srcSerNum andDestSerNum:destSerNum];
    return ECCP_Success;
}


- (NSInteger)acceptCall:(NSString *)digid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }

    [self enableLoudsSpeaker:NO];  //关闭扬声器播放来电铃声
    [self stopRing:NO];
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = acceptCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding]);
        if(ret != 0)
            return ECCP_Failed;

        runningType = ERunningType_Voip;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

//V2.1
- (NSInteger)acceptCall:(NSString*)digid withType:(NSInteger)callType
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }

    [self enableLoudsSpeaker:NO];  //关闭扬声器播放来电铃声
    [self stopRing:NO];
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = acceptCallByMediaType([call.callID cStringUsingEncoding:NSUTF8StringEncoding], callType);
        if(ret != 0)
            return ECCP_Failed;

        runningType = ERunningType_Voip;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

- (NSInteger)rejectCall:(NSString *)digid andReason:(NSInteger) reason
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    self.isCallAndRing = YES;
    VoipCall *call = [self getVoipCallByDigID:digid];
    if (reason == EReasonBusy) {
        //暂时啥都不做
    }
    else
    {
        runningType = ERunningType_None;
        [self enterVoipCallFlow:FALSE];
        [self enableLoudsSpeaker:NO];  //关闭扬声器播放来电铃声
    }

    [self stopRing:YES];
    if(call)
    {
        int ret = rejectCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], reason);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

//获取呼叫的媒体类型
- (NSInteger)getCallMediaType:(NSString*)digid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom)
    {
        return ECCP_Failed;
    }

    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = getCallMediaType([call.callID cStringUsingEncoding:NSUTF8StringEncoding]);
        return ret;
    }
    else
    {
        return ECCP_Failed;
    }
}

//更新已存在呼叫的媒体类型
- (NSInteger)requestSwitchCall:(NSString*)digid toMediaType:(NSInteger)callType
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }

    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = updateCallMedia([call.callID cStringUsingEncoding:NSUTF8StringEncoding], callType);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

//回复对方的更新请求
//1 同意  0 拒绝
- (NSInteger)responseSwitchCallMediaType:(NSString*)digid withAction:(NSInteger)action
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }

    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = answerCallMediaUpdate([call.callID cStringUsingEncoding:NSUTF8StringEncoding], action);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

- (NSInteger)pauseCall:(NSString *)digid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = pauseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding]);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

- (NSInteger)resumeCall:(NSString *)digid
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = resumeCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding]);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

-(NSString*)getCurrentCall
{
    const char * callid =  getCurrentCall();
    if (callid != NULL)
    {
        NSString* str = [NSString stringWithUTF8String:callid];
        if ([str length] > 0)
        {
            VoipCall *call = [voipCallDict valueForKey:str];
            if (call)
            {
                return call.digID;
            }
        }
    }

    return nil;
}



- (NSInteger)transferCall:(NSString *)digid withTransferID:(NSString *)destination
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = transferCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], [destination cStringUsingEncoding:NSUTF8StringEncoding]);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

- (NSInteger)transferCall:(NSString*)digid withTransferID:(NSString *)destination andType:(NSInteger) type;
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret = transferCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], [destination cStringUsingEncoding:NSUTF8StringEncoding],type);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

- (NSInteger)releaseCall:(NSString *)digid andReason:(NSInteger) reason
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    runningType = ERunningType_None;
    [self stopRing:YES];
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(!call)
        return ECCP_Failed;

    if(self.registerVoipCall)
    {
        [self onCallReleased:call.callID];
        self.registerVoipCall = nil;
        return ECCP_Success;
    }
    else
    {
        if(call.callStatus == ECallFailedBusy)
        {
            [self onCallReleased:call.callID];
            return ECCP_Success;
        }
        else
        {
            call.callStatus = ECallReleasing;
            int ret = releaseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], reason);
            if(ret != 0)
            {
                [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] releaseCall digid=%@\r\n", digid]];
                [voipCallDict removeObjectForKey:call.callID];
                return ECCP_Failed;
            }
            return ECCP_Success;
        }
    }
}
- (NSInteger)releaseCall:(NSString *)digid
{
    return [self releaseCall:digid andReason:0];
}

- (NSInteger)sendDTMF:(NSString *)digid dtmf:(NSString *)dtmf
{
    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        return ECCP_Failed;
    }
    VoipCall *call = [self getVoipCallByDigID:digid];
    if(call)
    {
        int ret =  sendDTMF([call.callID cStringUsingEncoding:NSUTF8StringEncoding], [dtmf characterAtIndex:0]);
        if(ret != 0)
            return ECCP_Failed;
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}


//日志开关
- (void)enableLog:(BOOL)isEnable
{
    if(isEnable)
    {
        setLogLevel(LOG_LEVEL_DEBUG);
    }
    else
    {
        setLogLevel(LOG_LEVEL_NONE);
    }
}
//登陆状态
- (BOOL)isOnline
{
    return isRegisted;
}

#pragma mark - 基本设置函数
- (BOOL)getLoudsSpeakerStatus
{
    return getLoudsSpeakerStatus();
}

//开启或关闭扬声器
- (NSInteger)enableLoudsSpeaker:(BOOL)enable
{
    int ret = enableLoudsSpeaker(enable);
    if(ret != 0)
        return ECCP_Failed;
    return ECCP_Success;
}

//静音或取消静音
- (NSInteger)setMute:(BOOL)on
{
    setMute(on);
    return ECCP_Success;
}

//获取静音
- (BOOL)getMuteStatus
{
    BOOL isMute = getMuteStatus();

    return isMute;
}

//来电铃声
- (NSInteger)setRing:(NSString *)ringName
{
    NSArray *aArray = [ringName componentsSeparatedByString:@"."];
    NSString *name = [aArray objectAtIndex:0];
    NSString *sufix = [aArray objectAtIndex:1];

    NSString *soundFilePath = [[NSBundle mainBundle] pathForResource:name ofType:sufix];
    if (soundFilePath.length <= 0)
    {
        return ECCP_Failed;
    }

    self.playRingName = soundFilePath;
    return ECCP_Success;
}

//呼叫回铃声
- (NSInteger)setRingback:(NSString *)ringName
{
    NSArray *aArray = [ringName componentsSeparatedByString:@"."];
    NSString *name = [aArray objectAtIndex:0];
    NSString *sufix = [aArray objectAtIndex:1];

    NSString *soundFilePath = [[NSBundle mainBundle] pathForResource:name ofType:sufix];
    if (soundFilePath.length <= 0)
    {
        return ECCP_Failed;
    }
    self.playRingbackName = soundFilePath;
    return ECCP_Success;
}
- (NSInteger)setVideoView:(UIView*)view andLocalView:(UIView*)localView
{
    int ret = setVideoView(view,localView);
    if(ret == 0)
    {
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

//获取摄像头信息
- (NSArray*)getCameraInfo
{
    CameraInfo * camerainfo;

    NSMutableArray *retArr = [[[NSMutableArray alloc] init] autorelease];
	int count = getCameraInfo(&camerainfo);
    for (int i = 0; i<count; i++)
    {
        CameraDeviceInfo * camera = [[CameraDeviceInfo alloc] init];

        CameraInfo * temp = camerainfo+i;
        camera.index = temp->index;
        camera.name = [NSString stringWithCString:temp->name encoding:NSUTF8StringEncoding];

        NSMutableArray *capabilityArr = [[NSMutableArray alloc] init];
        for (int j=0; j<temp->capabilityCount; j++)
        {
            CameraCapabilityInfo *capabilityInfo = [[CameraCapabilityInfo alloc] init];

            CameraCapability * tmp = temp->capability + j;
            capabilityInfo.width = tmp->width;
            capabilityInfo.height = tmp->height;
            capabilityInfo.maxfps = tmp->maxfps;

            [capabilityArr addObject:capabilityInfo];

            [capabilityInfo release];
        }

        camera.capabilityArray = capabilityArr.count>0? capabilityArr : nil;
        [capabilityArr release];

        [retArr addObject:camera];
        [camera release];
    }

    return retArr.count>0?retArr:nil;
}

#ifdef _custom_method_version_
- (void) notifyTo:(NSString *)receiver andVideoRotate:(NSInteger)degree
{
    NSString* tmp = [NSString stringWithFormat:@"%d",degree];
//    notifyVideoRotate([receiver cStringUsingEncoding:NSUTF8StringEncoding], [tmp cStringUsingEncoding:NSUTF8StringEncoding]);
}
#endif

//选取摄像头
- (NSInteger)selectCamera:(NSInteger)cameraIndex capability:(NSInteger)capabilityIndex fps:(NSInteger)fps rotate:(Rotate)rotate
{
    int ret = selectCamera(cameraIndex, capabilityIndex, fps, rotate,false);
    if(ret == 0)
    {
        return ECCP_Success;
    }
    else
    {
        return ECCP_Failed;
    }
}

-(void)setCodecEnabledWithCodec:(Codec) codec andEnabled:(BOOL) enabled
{
    setCodecEnabled(codec, enabled);
}

-(BOOL)getCondecEnabelWithCodec:(Codec) codec
{
    return getCodecEnabled(codec);
}

- (void)setUserAgent:(NSString *)agent
{
    setUserData(USERDATA_FOR_USER_AGENT, [agent cStringUsingEncoding:NSUTF8StringEncoding]);
}

#pragma mark - 播放语音函数

- (NSInteger)startPlayfile:(NSString *)filename withCallID:(NSString *)callID
{
    return ECCP_Failed;
}

- (NSInteger)stopPlayfile:(NSString *)callID
{
    return ECCP_Failed;
}

//日志
- (void)getRestLoginfo:(NSNotification *)notification
{
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getRestLoginfo %@",[[notification userInfo] objectForKey:SKD_LOG_KEY]]];
}

#pragma mark - rest登录返回的消息

//错误消息的统一处理
//返回值 statuscode 值
- (CloopenReason *)requestStatuscode:(NSNotification *)notification
{
    CloopenReason *reason = [[CloopenReason alloc] init];
    reason.reason = -1;
    if (notification.userInfo)
    {
        NSString *codeStr = [[notification userInfo] objectForKey:KEY_RESPONSE_STATUSCODE];
        reason.reason  = codeStr.length>0?codeStr.integerValue:ELocalReason_ErrorStutusCode;
        reason.msg = [[notification userInfo] objectForKey:KEY_RESPONSE_STATUSMSG];
    }
    return [reason autorelease];
}

-(void)editTestNumFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] editTestNumFinished statuscode=%d",status.reason]];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onEditTestNumWithReason:)])
    {
        [[g_CCPCallService delegate] onEditTestNumWithReason:status];
    }
}

-(NSString*)getLastLoginTime
{
    if ([self.lastTimeStr length]>0) {
         return [NSString stringWithFormat:@"%@",self.lastTimeStr];
    }
    else return nil;
}

- (void)getSipAddressFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    if (status.reason != 0)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onConnectError:withReasonMessge:)])
        {
            [[g_CCPCallService delegate] onConnectError:status.reason withReasonMessge:status.msg];
        }
        return;
    }

    NSString* nwgid = [[notification userInfo] objectForKey:NETWORK_GROUPID];
    if (nwgid.length > 0)
    {
        SetNetworkGroupId([nwgid cStringUsingEncoding:NSUTF8StringEncoding]);
    }

    NSString* strP2P = [[notification userInfo] objectForKey:SERVER_P2P_KEY];
    if ([strP2P length] > 0)
    {
        NSArray* arr = [strP2P componentsSeparatedByString:@":"];
        NSString* strIP = nil;
        int port = 0;
        if ([arr count ] == 2)
        {
            strIP = [arr objectAtIndex:0];
            port = [[arr objectAtIndex:1] intValue];
            [self setStunServerWithServer:strIP andPort:port];
        }
    }

    self.strCapability = [[notification userInfo] objectForKey:SERVER_STR_CAPABILITY];
    self.lastTimeStr = [[notification userInfo] objectForKey:KEY_LastLoginTime];
    ///////jiazy//////////

//    self.ccp_server_ip = @"192.168.175.125";
//    self.ccp_serverport = 7600;
//
////    self.account = @"8003703000000011";
////    self.password = @"YWAYVxL2";
//    self.account = @"1003";
//    self.password = @"1234";
////    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getSipAddressFinished serverip=%@ port=%d strP2P = %@ strCapability = %@", self.ccp_server_ip,self.ccp_serverport,strP2P,self.strCapability]];
////    setPrivateCloud("liqiang1090", NULL, false);
//    connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
//    return;

    ///////jiazy//////////

    NSString* clpssStr = [[notification userInfo] objectForKey:SERVER_GetSipAddressXML_KEY];
    if ([clpssStr length] > 0)
    {
        self.clpss = clpssStr;
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getSipAddressFinished serverXML=%@  strP2P = %@ strCapability = %@", self.clpss,strP2P,self.strCapability]];
        int ret = connectToCCPWithXML([self.clpss UTF8String],[self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
        if(ret !=0 && g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onConnectError:withReasonMessge:)])
        {
            [[g_CCPCallService delegate] onConnectError:ret withReasonMessge:@"连接云平台失败"];
        }
        return;
    }
    else
    {
        NSDictionary* dict = [[notification userInfo] objectForKey:SERVERIP_PORT_KEY];
        if (dict)
        {
            self.ccp_server_ip = [dict  objectForKey:@"ip"];
            self.ccp_serverport = [[dict objectForKey:@"port"] integerValue];
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getSipAddressFinished serverip=%@ port=%d strP2P = %@ strCapability = %@", self.ccp_server_ip,self.ccp_serverport,strP2P,self.strCapability]];
            connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
        }
    }
}

- (void)getNetGroupIdFinished:(NSNotification *)notification
{
//    CloopenReason * status = [self requestStatuscode:notification];
    NSString *nwgid = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    if (nwgid.length > 0)
    {
        SetNetworkGroupId([nwgid cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

- (void)getCallbackFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getCallbackFinished statuscode=%d",status.reason]];
    if (g_CCPCallService && [g_CCPCallService delegate] && [[g_CCPCallService delegate] respondsToSelector:@selector(onCallBackWithReason:andFrom:To:)])
    {
        [[g_CCPCallService delegate] onCallBackWithReason:status andFrom:[[notification userInfo] objectForKey:@"from"] To:[[notification userInfo] objectForKey:@"to"]];
    }
    /*
    if (statuscode != 0)
    {

        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onCallbackError:withReasonMessge:)])
        {
            [[g_CCPCallService delegate] onCallbackError:statuscode withReasonMessge:nil];
        }
        return;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getCallbackFinished"]];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onCallback)])
    {
        [[g_CCPCallService delegate] onCallback];
    }
     */
}


// 上传多媒体文件结果
-(void)uploadMediaMsgFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_SEND_MEDIAMSG_DATA];;

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] uploadMediaMsgFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onSendInstanceMessageWithReason:andMsg:)])
    {
        [[g_CCPCallService delegate] onSendInstanceMessageWithReason: status andMsg:data];
    }
}

//会议rest api完成回调
-(void)startinterphoneFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    NSString *interphoneId = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    if (status.reason == 0 && interphoneId.length > 0)
    {
        NSString *DigID = [self createDigID];

        NSString *name = self.userName.length>0?self.userName:@"";
        NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;", name];
        setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);

        const char *retCallid = makeCall(0,[interphoneId cStringUsingEncoding:NSUTF8StringEncoding]);

        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startinterphoneFinished makeCall=%@",interphoneId]];
        if(retCallid)
        {
            NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];

            self.interphoneCallId = CallID;

            VoipCall *call = [[VoipCall alloc] init];
            call.digID = DigID;
            call.callID = CallID;
            call.callDirect = EOutgoing;
            call.callType = 0;
            call.callStatus = ECallOutgoing;
            call.callee = interphoneId;
            [voipCallDict setValue:call forKey:CallID];
            [call release];
            return;
        }
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startinterphoneFinished statuscode=%d interphoneId=%@",status.reason,interphoneId]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
    {
        [[g_CCPCallService delegate] onInterphoneStateWithReason:status andConfNo:nil];
    }

    runningType = ERunningType_None;
}

-(void)controlMicFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id voip = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] controlMicFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onControlMicStateWithReason:andSpeaker:)])
    {
        [[g_CCPCallService delegate] onControlMicStateWithReason:status andSpeaker:voip];
    }

}

-(void)releaseMicFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onReleaseMicStateWithReason:)])
    {
        [[g_CCPCallService delegate] onReleaseMicStateWithReason:status];
    }
}

-(void)getInterphoneMemberListFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = nil;
    if (status.reason == 0)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getInterphoneMemberListFinished"]];
        data = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    }
    else
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getInterphoneMemberListFailed statuscode=%d",status.reason]];
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneMembersWithReason:andData:)])
    {
        [[g_CCPCallService delegate] onInterphoneMembersWithReason:status andData:data];
    }
}

-(void)startChatroomHttpFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    NSString *roomId = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    NSString *roomPwd = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOMPWD];
    NSNumber *isAutoJoin = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
    BOOL autoJoin = YES;
    if (isAutoJoin)
    {
        autoJoin = [isAutoJoin boolValue];
    }

    if (status.reason == 0 && roomId.length > 0)
    {
        if ([roomId length] > 30)
        {
            if (!self.longVoiceRoomNos)
            {
                self.longVoiceRoomNos = [[NSMutableArray alloc] init];
            }
            [self.longVoiceRoomNos addObject:roomId];
        }

        if (autoJoin)
        {
            NSString *DigID = [self createDigID];
            NSString *name = self.userName.length>0?self.userName:@"";
            NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;confpwd=%@", name,roomPwd];
            setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);
            const char *retCallid = makeCall(0,[roomId cStringUsingEncoding:NSUTF8StringEncoding]);
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startinterphoneFinished makeCall=%@",roomId]];
            if(retCallid)
            {
                NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];
                self.interphoneCallId = CallID;
                VoipCall *call = [[VoipCall alloc] init];
                call.digID = DigID;
                call.callID = CallID;
                call.callDirect = EOutgoing;
                call.callType = 0;
                call.callStatus = ECallOutgoing;
                call.callee = roomId;
                [voipCallDict setValue:call forKey:CallID];
                [call release];
                return;
            }
        }
    }
    NSString* confNo = nil;
    if ([roomId length] > 30)
    {
        confNo = [roomId substringToIndex:30];
    }
    else
        confNo = roomId;

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startinterphoneFinished statuscode=%d interphoneId=%@",status.reason,confNo]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
    {
        [[g_CCPCallService delegate] onChatroomStateWithReason:status andRoomNo:confNo];
    }

    runningType = ERunningType_None;
}

-(void)getChatroomslistFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = nil;
    if (status.reason == 0)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getChatroomslistFinished"]];
        data = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    }
    else
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getChatroomslistFailed statuscode=%d",status.reason]];
    }
    self.longVoiceRoomNos = nil;
    self.longVoiceRoomNos = [[NSMutableArray alloc] init];
    NSMutableArray *roomsInfoArray = data;
    for (Chatroom *roomInfo in roomsInfoArray)
    {
        if ([roomInfo.roomNo length] > 30)
        {
            NSString* strRoomNo = [NSString stringWithFormat:@"%@",roomInfo.roomNo];
            [self.longVoiceRoomNos addObject:strRoomNo];
            roomInfo.roomNo = [roomInfo.roomNo substringToIndex:30];
        }
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomsWithReason:andRooms:)])
    {
        [[g_CCPCallService delegate] onChatroomsWithReason:status andRooms:data];
    }
}

-(void)inviteMembersToChatroomFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_CHATROOMNO];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getChatroomslistFailed statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomInviteMembersWithReason:andRoomNo:)])
    {
        [[g_CCPCallService delegate] onChatroomInviteMembersWithReason:status andRoomNo:data];
    }
}

-(void)getChatRoomMemberListFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = nil;
    if (status.reason == 0)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getChatRoomMemberListFinished"]];
        data = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    }
    else
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getChatRoomMemberListFinished statuscode=%d",status.reason]];
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomMembersWithReason:andData:)])
    {
        [[g_CCPCallService delegate] onChatroomMembersWithReason:status andData:data];
    }
}

- (void) dismissChatRoomFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_CHATROOMNO];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] dismissChatRoomFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomDismissWithReason:andRoomNo:)])
    {
        [[g_CCPCallService delegate] onChatroomDismissWithReason:status andRoomNo:data];
    }
}

- (void) removeMemberChatRoomFinished:(NSNotification *) notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_MEMBERDATA];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] removeMemberChatRoomFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomRemoveMemberWithReason:andMember:)])
    {
        [[g_CCPCallService delegate] onChatroomRemoveMemberWithReason:status andMember:data];
    }
}

-(void)startVideoConferenceHttpFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    NSString *roomId = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    NSNumber *isAutoJoin = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
    BOOL autoJoin = YES;
    if (isAutoJoin)
    {
        autoJoin = [isAutoJoin boolValue];
    }

    if (status.reason == 0 && roomId.length > 0)
    {
        if ([roomId length] >30)
        {
            if (!self.longVideoRoomNos)
            {
                self.longVideoRoomNos = [[NSMutableArray alloc] init];
            }
            [self.longVideoRoomNos addObject:roomId];
        }

        if (autoJoin)
        {
            NSString *DigID = [self createDigID];
            NSString *name = self.userName.length>0?self.userName:@"";
            NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;", name];
            setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);
            const char *retCallid = makeCall(1,[roomId cStringUsingEncoding:NSUTF8StringEncoding]);
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startvideoconferenceFinished makeCall=%@",roomId]];
            if(retCallid)
            {
                NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];
                self.interphoneCallId = CallID;
                VoipCall *call = [[VoipCall alloc] init];
                call.digID = DigID;
                call.callID = CallID;
                call.callDirect = EOutgoing;
                call.callType = 0;
                call.callStatus = ECallOutgoing;
                call.callee = roomId;
                [voipCallDict setValue:call forKey:CallID];
                [call release];
                return;
            }
        }
    }

    NSString* confNo = nil;
    if ([roomId length] > 30)
    {
        confNo = [roomId substringToIndex:30];
    }
    else
        confNo = roomId;

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startvideoconferenceFinished statuscode=%d roomId=%@",status.reason,confNo]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
    {
        [[g_CCPCallService delegate] onVideoConferenceStateWithReason:status andConferenceId:confNo];
    }

    runningType = ERunningType_None;
}

-(void)startMultiVideoHttpFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    NSString *roomId = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    NSNumber *isAutoJoin = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
    BOOL autoJoin = YES;
    if (isAutoJoin)
    {
        autoJoin = [isAutoJoin boolValue];
    }

    if (status.reason == 0 && roomId.length > 0)
    {
        if ([roomId length] >30)
        {
            if (!self.longVideoRoomNos)
            {
                self.longVideoRoomNos = [[NSMutableArray alloc] init];
            }
            [self.longVideoRoomNos addObject:roomId];
        }

        if (autoJoin)
        {
            NSString *DigID = [self createDigID];
            NSString *name = self.userName.length>0?self.userName:@"";
            NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;", name];
            setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);
            const char *retCallid = makeCall(1,[roomId cStringUsingEncoding:NSUTF8StringEncoding]);
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startmultivideoFinished makeCall=%@",roomId]];
            if(retCallid)
            {
                NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];
                self.interphoneCallId = CallID;
                VoipCall *call = [[VoipCall alloc] init];
                call.digID = DigID;
                call.callID = CallID;
                call.callDirect = EOutgoing;
                call.callType = 0;
                call.callStatus = ECallOutgoing;
                call.callee = roomId;
                [voipCallDict setValue:call forKey:CallID];
                [call release];
                return;
            }
        }
    }

    NSString* confNo = nil;
    if ([roomId length] > 30)
    {
        confNo = [roomId substringToIndex:30];
    }
    else
        confNo = roomId;

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] startimultivideoFinished statuscode=%d roomId=%@",status.reason,confNo]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
    {
        [[g_CCPCallService delegate] onVideoConferenceStateWithReason:status andConferenceId:confNo];
    }

    runningType = ERunningType_None;
}

-(void)getVideoConferenceslistFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = nil;
    if (status.reason == 0)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getVideoConferenceslistFinished"]];
        data = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    }
    else
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getVideoConferenceslistFailed statuscode=%d",status.reason]];
    }
    self.longVideoRoomNos = nil;
    self.longVideoRoomNos = [[NSMutableArray alloc] init];
    NSMutableArray *roomsInfoArray = data;
    for (VideoConference *roomInfo in roomsInfoArray)
    {
        if ([roomInfo.conferenceId length] > 30)
        {
            NSString* strRoomNo = [NSString stringWithFormat:@"%@",roomInfo.conferenceId];
            [self.longVideoRoomNos addObject:strRoomNo];
            roomInfo.conferenceId = [roomInfo.conferenceId substringToIndex:30];
        }
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferencesWithReason:andConferences:)])
    {
        [[g_CCPCallService delegate] onVideoConferencesWithReason:status andConferences:data];
    }
}

-(void)getVideoConferenceMemberListFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = nil;
    if (status.reason == 0)
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getVideoConferenceMemberListFinished"]];
        data = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];
    }
    else
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getVideoConferenceMemberListFinished statuscode=%d",status.reason]];
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceMembersWithReason:andData:)])
    {
        [[g_CCPCallService delegate] onVideoConferenceMembersWithReason:status andData:data];
    }
}

- (void) dismissVideoConferenceFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_VIDEOCONFERENCEID];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] dismissVideoConferenceFinished statuscode=%d",status.reason]];
    [self exitVideoConference];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceDismissWithReason:andConferenceId:)])
    {
        [[g_CCPCallService delegate] onVideoConferenceDismissWithReason:status andConferenceId:data];
    }
}

- (void) removeMemberVideoConferenceFinished:(NSNotification *) notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_MEMBERDATA];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] removeMemberVideoConferenceFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceRemoveMemberWithReason:andMember:)])
    {
        [[g_CCPCallService delegate] onVideoConferenceRemoveMemberWithReason:status andMember:data];
    }
}

- (void) switchRealScreenFinished:(NSNotification *) notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] onSwitchRealScreenToVoipWithReason statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onSwitchRealScreenToVoipWithReason:)])
    {
        [[g_CCPCallService delegate] onSwitchRealScreenToVoipWithReason:status];
    }
}

-(void)publishVideoInMultiVideoConference:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] publishVideoInVideoConference statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onPublishVideoInVideoConferenceWithReason:)])
    {
        [[g_CCPCallService delegate] onPublishVideoInVideoConferenceWithReason:status];
    }
}

-(void)unpublishVideoInMultiVideoConference:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] unpublishVideoInVideoConference statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onUnpublishVideoInVideoConferenceWithReason:)])
    {
        [[g_CCPCallService delegate] onUnpublishVideoInVideoConferenceWithReason:status];
    }
}

// 上传头像文件结果
-(void)sendVideoConferencePortraitMsgFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_MEDIAMSG_FILENAME];

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] sendVideoConferenceMsgFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onSendLocalPortraitWithReason:andPortrait:)])
    {
        [[g_CCPCallService delegate] onSendLocalPortraitWithReason:status andPortrait:data];
    }
}

//获取头像列表
-(void)getVideoConferencePortraitsMsgFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_RESPONSE_USERDATA];

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] getVideoConferencePortraitsMsgFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onGetPortraitsFromVideoConferenceWithReason:andPortraitList:)])
    {
        [[g_CCPCallService delegate] onGetPortraitsFromVideoConferenceWithReason:status andPortraitList:data];
    }
}

// 下载头像文件回调
- (void)downloadVideoPortraitMsgsFinished:(NSNotification *)notification
{
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] downloadVideoPortraitMsgsFinished"]];
    id info = [notification.userInfo objectForKey:KEY_USERINFO_DOWNLOADFILEINFO];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onDownloadVideoConferencePortraitsWithReason:andPortrait:)])
    {
        [[g_CCPCallService delegate] onDownloadVideoConferencePortraitsWithReason:0 andPortrait:info];
    }
}
//#pragma mark - 播音和录音
//录音超时
-(void)recordingTimeout
{
    [self stopVoiceRecording];
    if (self.delegate && [self.delegate respondsToSelector:@selector(onRecordingTimeOut:)])
        [self.delegate onRecordingTimeOut:RECORD_TIMEOUT*1000];
}

-(void)startRecordingChunkedWithFileName:(NSString*) fileName andReceiver:(NSString*)receiver andChunked:(BOOL)chunked andUserdata:(NSString*)userdata andSendMsgId:(NSString*)sendMsgid
{
    Consumer* consumer  = [[Consumer alloc] init];
    consumer.fileName = fileName;
    consumer.receiver = receiver;
    consumer.userData = userdata;
    consumer.MsgId = sendMsgid;
    recorder->setConsumer(consumer);
    consumer.myDelegate = self;
    [sendTask setObject:consumer forKey:fileName];
    [consumer release];
    if (recorder->IsRunning()) // If we are currently recording, stop and save the file.
    {
        [self stopVoiceRecording];
    }
    else // If we're not recording, start.
    {
        recorder->StartRealTimeRecord((CFStringRef)fileName);
        [self setFileDescriptionForFormat:recorder->DataFormat() withName:@"Recorded File"];

        NSTimer *timer1 = [NSTimer scheduledTimerWithTimeInterval:RECORD_TIMEOUT target:self selector:@selector(recordingTimeout) userInfo:nil repeats:NO];
        self.recordTimer = timer1;
        NSTimer *timer2 = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(sendAmplitude) userInfo:nil repeats:YES];
        self.decibelsTimer = timer2;
    }
}

-(void)startRecordingWithFileName:(NSString*) fileName
{
   // NSLog(@"==== recordButton clicked ====\n");
    if (recorder->IsRunning()) // If we are currently recording, stop and save the file.
    {
        [self stopVoiceRecording];
    }
    else // If we're not recording, start.
    {
        recorder->StartRecord((CFStringRef)fileName);
        [self setFileDescriptionForFormat:recorder->DataFormat() withName:@"Recorded File"];

        NSTimer *timer1 = [NSTimer scheduledTimerWithTimeInterval:RECORD_TIMEOUT target:self selector:@selector(recordingTimeout) userInfo:nil repeats:NO];
        self.recordTimer = timer1;
        NSTimer *timer2 = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(sendAmplitude) userInfo:nil repeats:YES];
        self.decibelsTimer = timer2;
    }
}

char *OSTypeToStr(char *buf, OSType t)
{
	char *p = buf;
	char str[4], *q = str;
	*(UInt32 *)str = CFSwapInt32(t);
	for (int i = 0; i < 4; ++i) {
		if (isprint(*q) && *q != '\\')
			*p++ = *q++;
		else {
			sprintf(p, "\\x%02x", *q++);
			p += 4;
		}
	}
	*p = '\0';
	return buf;
}

-(void)setFileDescriptionForFormat: (CAStreamBasicDescription)format withName:(NSString*)name
{
	char buf[5];
	const char *dataFormat = OSTypeToStr(buf, format.mFormatID);
//	NSString* description = [[NSString alloc] initWithFormat:@"(%d ch. %s @ %g Hz)", format.NumberChannels(), dataFormat, format.mSampleRate, nil];
//    //NSLog(@"record file description: %@\n", description);
//    [self onLogInfo:[NSString stringWithFormat:@"record file description: %@\n", description]];
//	[description release];
}

// 停止录音
-(void)stopVoiceRecording
{
    if (self.recordTimer)
    {
        [self.recordTimer invalidate];
        self.recordTimer = nil;
    }
    if (self.decibelsTimer)
    {
        [self.decibelsTimer invalidate];
        self.decibelsTimer = nil;
    }
    [self performSelector:@selector(stopRecordingDelayed) withObject:nil afterDelay:.3];
}

-(void)stopRecordingDelayed
{
    if (recorder->IsRunning())
        recorder->StopRecord(NO);
}

// 播放语音文件
-(void)playVoiceMsg:(NSString*) fileName
{
    if (self.player)
        [self.player stop];

    NSFileManager* manager = [NSFileManager defaultManager];
    if (![manager fileExistsAtPath:fileName])
    {
        return;
    }
    NSError *err;
    NSString *filePath1 = fileName;
    NSString *filePath2 = [NSTemporaryDirectory() stringByAppendingPathComponent: @"tempPlaying.wav"];
    NSData * amrData =[NSData dataWithContentsOfFile:filePath1];
    if (!amrData) {
        return;
    }
    NSData * wavData = DecodeAMRToWAVE(amrData);
    if (!wavData) {
        return;
    }
    [wavData writeToFile:filePath2 atomically:YES];
    self.player=[[[AVAudioPlayer alloc]initWithContentsOfURL:[NSURL fileURLWithPath:filePath2] error:&err] autorelease];
    self.player.delegate=self;
    if (self.player==nil)
    {
        [self onLogInfo:[NSString stringWithFormat:@"ERror creating player:%@", [err description]]];
        return;
    }
    self.player.volume = 1.0;
    //[self enableLoudsSpeaker:YES];
    [self.player prepareToPlay];
    [self.player play];
    isVoiceMsgPlaying = YES;
    self.player.delegate = self;
}


// 停止当前播放语音
-(void)stopVoiceMsg
{
    isVoiceMsgPlaying = NO;
    [self enableLoudsSpeaker:NO];
    [self.player stop];
}


-(void)sendAmplitude
{
    double ret = 0;
    if (recorder->IsRunning())
    {
        ret = recorder ->getPeakPower();
    }
    else
    {
        return;
    }

	const double ALPHA = 0.05;
	double peakPowerForChannel = pow(10, (0.05 * ret));
	lowPassResults = ALPHA * peakPowerForChannel + (1.0 - ALPHA) * lowPassResults;

    //    if (lowPassResults < 0.25)//可以过滤掉太低的声音
    //        return;
    if (recorder ->IsRunning() && self.delegate && [self.delegate respondsToSelector:@selector(onRecordingAmplitude:)])
        [self.delegate onRecordingAmplitude: lowPassResults];
}

// 获取语音文件的播放时长
-(long)getVoiceDuration:(NSString*) fileName
{
    NSFileManager* manager = [NSFileManager defaultManager];
    if ([manager fileExistsAtPath:fileName])
    {
        return [[manager attributesOfItemAtPath:fileName error:nil] fileSize] *1000 / 650;
    }
    return 0;
//    NSString *filePath2 = [NSTemporaryDirectory() stringByAppendingPathComponent: @"tempPlaying.wav"];
//    NSData * amrData =[NSData dataWithContentsOfFile:filePath1];
//    NSData * wavData = DecodeAMRToWAVE(amrData);
//    [wavData writeToFile:filePath2 atomically:YES];
//
//    NSError *playerError;
//    NSURL *recordedFile;
//    long Duration = 0;
//    recordedFile = [NSURL fileURLWithPath:filePath2];
//
//    AVAudioPlayer *playerDuration = [[AVAudioPlayer alloc] initWithContentsOfURL:recordedFile error:&playerError];
//    if (playerDuration)
//    {
//        Duration = [playerDuration duration]*1000;
//    }
//    else
//        [self onLogInfo:[NSString stringWithFormat:@"ERror creating player:%@", [playerError description]]];
//    [playerDuration release];
}

#pragma mark - 实时对讲相关函数
//启动实时对讲
- (void) startInterphoneWithJoiner:(NSArray*)members inAppId:(NSString*)appId
{
    if (runningType != ERunningType_None)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onInterphoneStateWithReason:reason andConfNo:nil];
            [reason release];
        }
        return;
    }
    if (isRegisted)
    {
        runningType = ERunningType_Interphone;
        [self.ccpRestService startInterphoneWithJoiner:members inAppId:appId];
    }
    else
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_NotRegisted;
            reason.msg = @"未登录";
            [[g_CCPCallService delegate] onInterphoneStateWithReason:reason andConfNo:nil];
            [reason release];
        }
    }
}

//加入实时对讲
- (void) joinInterphoneToConfNo:(NSString*)confNo
{
    if (runningType != ERunningType_None)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onInterphoneStateWithReason:reason andConfNo:nil];
            [reason release];
        }
        return;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] joinInterphoneToConfNo"]];

    runningType = ERunningType_Interphone;

    NSString *name = self.userName.length>0?self.userName:@"";
    NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;", name];
    setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);

    if (confNo.length > 0)
    {
        NSString *DigID = [self createDigID];
        const char *retCallid = makeCall(0,[confNo cStringUsingEncoding:NSUTF8StringEncoding]);

        if(retCallid)
        {
            NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];

            self.interphoneCallId = CallID;

            VoipCall *call = [[VoipCall alloc] init];
            call.digID = DigID;
            call.callID = CallID;
            call.callDirect = EOutgoing;
            call.callType = 0;
            call.callStatus = ECallOutgoing;
            call.callee = confNo;
            [voipCallDict setValue:call forKey:CallID];
            [call release];
            return;
        }
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onInterphoneStateWithReason:andConfNo:)])
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] joinInterphoneToConfNo delegate"]];
        CloopenReason *creason = [[CloopenReason alloc] init];
        creason.reason = ELocalReason_JoinFailed;
        creason.msg = @"加入失败";
        [[g_CCPCallService delegate] onInterphoneStateWithReason:creason andConfNo:confNo];
        [creason release];
    }
    runningType = ERunningType_None;
}

//退出实时对讲
- (BOOL) exitInterphone
{
    [self enableLoudsSpeaker:NO];
    if (runningType != ERunningType_Interphone)
    {
        return NO;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] releaseCall exitInterphone\r\n"]];
    VoipCall* call = [voipCallDict valueForKey:self.interphoneCallId];
    runningType = ERunningType_None;
    if (call)
    {
        releaseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], 0);
        [voipCallDict removeObjectForKey:call.callID];
    }
    return YES;
}

//发起实时对讲——抢麦
- (void) controlMicInConfNo:(NSString*)confNo
{
    if (runningType == ERunningType_Interphone)
    {
        [self.ccpRestService controlMicInConfNo:confNo];
    }
    else
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onControlMicStateWithReason:andSpeaker:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onControlMicStateWithReason:reason andSpeaker:nil];
            [reason release];
        }
    }
}

//结束实时对讲——放麦
- (void) releaseMicInConfNo:(NSString*)confNo
{
    if (runningType == ERunningType_Interphone)
    {
        [self.ccpRestService releaseMicInConfNo:confNo];
    }
    else
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onReleaseMicStateWithReason:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onReleaseMicStateWithReason:reason];
            [reason release];
        }
    }
}

//查询参与对讲成员
- (void) queryMembersWithInterphone:(NSString*)confNo
{
    [self.ccpRestService getMemberListInConfNo:confNo];
}

#pragma mark - 聊天室相关函数
//创建聊天室
- (void) startChatroomInAppId:(NSString *)appId withName:(NSString *)roomName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)roomPwd andIsAutoClose:(BOOL)isAutoClose andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL) autoDelete andIsAutoJoin:(BOOL) isAutoJoin;
{
    if (runningType != ERunningType_None && isAutoJoin)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onChatroomStateWithReason:reason andRoomNo:nil];
            [reason release];
        }
        return;
    }
    if (isRegisted)
    {
        if (isAutoJoin)
        {
            runningType = ERunningType_ChatRoom;
        }
        NSString *strPwd = @"";
        if ([roomPwd length] >0)
        {
            strPwd = [CLOPEncrypt clop_encodedData:roomPwd andPrivateKey:@"10C173A98BAD2FF723BE0E81A9D85965"];
        }
        [self.ccpRestService startChatroomInAppId:appId withName:roomName andSquare:square andKeywords:keywords andPassword:strPwd andIsAutoClose:isAutoClose andVoiceMod:voiceMod andAutoDelete:autoDelete andIsAutoJoin:isAutoJoin];
    }
    else
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_NotRegisted;
            reason.msg = @"未登录";
            [[g_CCPCallService delegate] onChatroomStateWithReason:reason andRoomNo:nil];
            [reason release];
        }
    }
}

//加入会议
- (void) joinChatroomInRoom:(NSString *)roomNo andPwd:(NSString*) pwd
{
    if (runningType != ERunningType_None)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onChatroomStateWithReason:reason andRoomNo:nil];
            [reason release];
        }
        return;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] joinChatroomInRoom"]];

    NSString* tempStrRoomNo = nil;
    for (NSString*  strRoomNo in self.longVoiceRoomNos)
    {
        if ([roomNo isEqualToString:[strRoomNo substringToIndex:30]])
        {
            tempStrRoomNo = [NSString stringWithFormat:@"%@",strRoomNo];
            break;
        }
    }

    if ([tempStrRoomNo length] <=0)
    {
        tempStrRoomNo = roomNo;
    }

    runningType = ERunningType_ChatRoom;
    NSString *name = self.userName.length>0?self.userName:@"";
    NSString *strPwd = @"";
    if ([pwd length] >0)
    {
        strPwd = [CLOPEncrypt clop_encodedData:pwd andPrivateKey:@"10C173A98BAD2FF723BE0E81A9D85965"];
    }
    NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;confpwd=%@", name,strPwd];
    setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);

    if (roomNo.length > 0)
    {
        NSString *DigID = [self createDigID];
        const char *retCallid = makeCall(0,[tempStrRoomNo cStringUsingEncoding:NSUTF8StringEncoding]);

        if(retCallid)
        {
            NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];

            self.interphoneCallId = CallID;

            VoipCall *call = [[VoipCall alloc] init];
            call.digID = DigID;
            call.callID = CallID;
            call.callDirect = EOutgoing;
            call.callType = 0;
            call.callStatus = ECallOutgoing;
            call.callee = tempStrRoomNo;
            [voipCallDict setValue:call forKey:CallID];
            [call release];
            return;
        }
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onChatroomStateWithReason:andRoomNo:)])
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] joinChatroomInRoom delegate"]];
        CloopenReason *creason = [[CloopenReason alloc] init];
        creason.reason = ELocalReason_JoinFailed;
        creason.msg = @"加入失败";
        [[g_CCPCallService delegate] onChatroomStateWithReason:creason andRoomNo:roomNo];
        [creason release];
    }
    runningType = ERunningType_None;
}

//退出会议
- (BOOL) exitChatroom
{
    [self enableLoudsSpeaker:NO];
    if (runningType != ERunningType_ChatRoom)
    {
        return NO;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] releaseCall exitChatRoom\r\n"]];
    VoipCall* call = [voipCallDict valueForKey:self.interphoneCallId];
    runningType = ERunningType_None;
    if (call)
    {
        releaseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], 0);
        [voipCallDict removeObjectForKey:call.callID];
    }
    return YES;
}

//获取会议成员
- (void) queryMembersWithChatroom:(NSString *)roomNo
{
    [self.ccpRestService queryMembersWithChatroom:roomNo];
}

//获取所有的房间列表
- (void)queryChatroomsOfAppId:(NSString *)appId withKeywords:(NSString *)keywords
{
    [self.ccpRestService queryChatroomsOfAppId:appId withKeywords:keywords];
}

//外呼邀请成员加入群聊
- (void)inviteMembers:(NSArray*)members joinChatroom:(NSString*)roomNo ofAppId:(NSString *)appId
{
    [self.ccpRestService inviteMembers:members joinChatroom:roomNo ofAppId:appId];
}

/**
 * 解散聊天室
 */
- (void) dismissChatroomWithAppId:(NSString*) appId andRoomNo:(NSString*) roomNo
{
    [self.ccpRestService dismissChatroom:roomNo ofAppId:appId];
}

/**
 * 踢出聊天室成员
 */
- (void) removeMemberFromChatroomWithAppId:(NSString*) appId andRoomNo:(NSString*) roomNo andMember:(NSString*) member
{
    [self.ccpRestService removeMember:member fromRoomNo:roomNo ofAppId:appId];
}


#pragma mark - 视频会议 video conference
/**
 * 创建视频会议
 * @param appId 应用id
 * @param conferenceName 会议名称
 * @param square 参与的最大方数
 * @param keywords 业务属性，有应用定义
 * @param conferencePwd 房间密码，可为null
 */
- (void) startVideoConferenceInAppId:(NSString*)appId withName:(NSString *)conferenceName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)conferencePwd andIsAutoClose:(BOOL)isAutoClose andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL) autoDelete andIsAutoJoin:(BOOL) isAutoJoin;
{
    if (runningType != ERunningType_None && isAutoJoin)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:nil];
            [reason release];
        }
        return;
    }
    if (isRegisted)
    {
        if (isAutoJoin)
        {
            runningType = ERunningType_VideoConference;
        }
        [self.ccpRestService startVideoConferenceInAppId:appId withName:conferenceName andSquare:square andKeywords:keywords andPassword:conferencePwd andIsAutoClose:isAutoClose andVoiceMod:voiceMod andAutoDelete:autoDelete andIsAutoJoin:isAutoJoin];
    }
    else
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_NotRegisted;
            reason.msg = @"未登录";
            [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:nil];
            [reason release];
        }
    }
}

/**
 * 加入视频会议
 * @param conferenceId 会议号
 */
- (void) joinInVideoConference:(NSString *)conferenceId
{
    if (runningType != ERunningType_None)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:nil];
            [reason release];
        }
        return;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] joinInVideoConference"]];

    NSString* tempStrRoomNo = nil;
    for (NSString*  strRoomNo in self.longVideoRoomNos)
    {
        if ([conferenceId isEqualToString:[strRoomNo substringToIndex:30]])
        {
            tempStrRoomNo = [NSString stringWithFormat:@"%@",strRoomNo];
            break;
        }
    }

    if ([tempStrRoomNo length] <=0)
    {
        tempStrRoomNo = conferenceId;
    }

    runningType = ERunningType_VideoConference;
    NSString *name = self.userName.length>0?self.userName:@"";
    NSString *userdata = [NSString stringWithFormat:@"tel=;nickname=%@;", name];
    setUserData(USERDATA_FOR_INVITE, [userdata cStringUsingEncoding:NSUTF8StringEncoding]);

    if (conferenceId.length > 0)
    {
        NSString *DigID = [self createDigID];
        const char *retCallid = makeCall(1,[tempStrRoomNo cStringUsingEncoding:NSUTF8StringEncoding]);

        if(retCallid)
        {
            NSString *CallID = [NSString stringWithCString:retCallid encoding:NSUTF8StringEncoding];
            self.interphoneCallId = CallID;
            VoipCall *call = [[VoipCall alloc] init];
            call.digID = DigID;
            call.callID = CallID;
            call.callDirect = EOutgoing;
            call.callType = 0;
            call.callStatus = ECallOutgoing;
            call.callee = tempStrRoomNo;
            [voipCallDict setValue:call forKey:CallID];
            [call release];
            return;
        }
    }

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
    {
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] joinInVideoConference delegate"]];
        CloopenReason *creason = [[CloopenReason alloc] init];
        creason.reason = ELocalReason_JoinFailed;
        creason.msg = @"加入失败";
        [[g_CCPCallService delegate] onVideoConferenceStateWithReason:creason andConferenceId:conferenceId];
        [creason release];
    }
    runningType = ERunningType_None;
}

/**
 * 退出当前视频会议
 * @return false:失败 true:成功
 */
- (BOOL) exitVideoConference
{
    [self enableLoudsSpeaker:NO];
    if (runningType != ERunningType_VideoConference)
    {
        return NO;
    }

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] releaseCall exitVideoConference\r\n"]];
    VoipCall* call = [voipCallDict valueForKey:self.interphoneCallId];
    runningType = ERunningType_None;
    if (call)
    {
        releaseCall([call.callID cStringUsingEncoding:NSUTF8StringEncoding], 0);
        [voipCallDict removeObjectForKey:call.callID];
    }
    return YES;
}

/**
 * 查询视频会议成员
 * @param conferenceId 会议号
 */
- (void) queryMembersInVideoConference:(NSString *)conferenceId
{
    [self.ccpRestService queryMembersInVideoConference:conferenceId];
}

/**
 * 查询视频会议列表
 * @param appId 应用id
 * @param keywords 业务属性
 */
- (void)queryVideoConferencesOfAppId:(NSString *)appId withKeywords:(NSString *)keywords
{
    [self.ccpRestService queryVideoConferencesOfAppId:appId withKeywords:keywords];
}

/**
 * 解散视频会议
 * @param appId 应用id
 * @param conferenceId 会议号
 */
- (void) dismissVideoConferenceWithAppId:(NSString*) appId andVideoConference:(NSString*) conferenceId
{
    [self.ccpRestService dismissVideoConferenceWithAppId:appId andVideoConference:conferenceId];
}

/**
 * 踢出视频会议成员
 * @param appId 应用id
 * @param conferenceId 会议号
 * @param member 成员号码
 */
- (void) removeMemberFromVideoConferenceWithAppId:(NSString*) appId andVideoConference:(NSString*)conferenceId andMember:(NSString*) member
{
    [self.ccpRestService removeMemberFromVideoConferenceWithAppId:appId andVideoConference:conferenceId andMember:member];
}

/**
 * 切换实时视频显示
 * @param voip 实时图像的voip
 * @param conferenceId 会议号
 * @param appId 应用id
 */
- (void) switchRealScreenToVoip:(NSString*)voip ofVideoConference:(NSString*)conferenceId inAppId:(NSString*)appId
{
    [self.ccpRestService switchRealScreenToVoip:voip ofVideoConference:conferenceId inAppId:appId];
}

- (void) sendLocalPortrait:(NSString*)filename toVideoConference:(NSString*)conferenceId
{
    [self.ccpRestService sendLocalPortrait:filename toVideoConference:conferenceId];
}

- (void) getPortraitsFromVideoConference:(NSString*)conferenceId
{
    [self.ccpRestService getPortraitsFromVideoConference:conferenceId];
}
- (void)downloadVideoConferencePortraits:(NSArray*)portraitsList
{
    for (id data in portraitsList)
    {
        [self.ccpRestService downloadVideoConferencePortrait:data];
    }
}
#pragma mark - multivideo method

- (void) startMultiVideoConferenceInAppId:(NSString*)appId withName:(NSString *)conferenceName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)conferencePwd andIsAutoClose:(BOOL)isAutoClose andIsPresenter:(BOOL)isPresenter andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL)autoDelete andIsAutoJoin:(BOOL) isAutoJoin
{
    if (runningType != ERunningType_None && isAutoJoin)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_CallBusy;
            reason.msg = @"呼叫线路忙";
            [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:nil];
            [reason release];
        }
        return;
    }
    if (isRegisted)
    {
        if (isAutoJoin)
        {
            runningType = ERunningType_VideoConference;
        }
        [self.ccpRestService startMultiVideoInAppId:appId withName:conferenceName andSquare:square andKeywords:keywords andPassword:conferencePwd andIsAutoClose:isAutoClose andIsPresenter:isPresenter andVoiceMod:voiceMod andAutoDelete:autoDelete andIsAutoJoin:isAutoJoin];
    }
    else
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onVideoConferenceStateWithReason:andConferenceId:)])
        {
            CloopenReason *reason = [[CloopenReason alloc] init];
            reason.reason = ELocalReason_NotRegisted;
            reason.msg = @"未登录";
            [[g_CCPCallService delegate] onVideoConferenceStateWithReason:reason andConferenceId:nil];
            [reason release];
        }
    }
}

/**
 * 设置视频会议服务器地址
 * @param addr 视频会议服务器ip
 * @param port 视频会议服务器端口
 * @return   成功 0 失败-1
 */
- (NSInteger)setVideoConferenceAddr:(NSString*)addr
{
    if (addr.length<=0)
    {
        return -1;
    }
    return setVideoConferenceAddr([addr UTF8String]);
}

/*! @function
 ********************************************************************************
 函数名   : requestMemberVideo
 功能     : 视频会议中请求某一远端视频
 参数     : [IN]  conferenceNo	  :  所在会议号.
 [IN]  conferencePasswd : 所在会议密码.
 [IN]  remoteSipNo     :  请求远端用户的sip号.
 [IN]  videoWindow     :  当成功请求时，展示该成员的窗口.
 返回值   : 成功 0 失败 -1(voipAccount为NULL) -2(displayView为NULL) -3(conferenceId为NULL) -5(自己的sip号为NULL) -6(会议服务器的ip为NULL) -7(该账户的视频已经成功请求)
 *******************************************************************************/
- (NSInteger)requestConferenceMemberVideoWithAccount:(NSString*)voipAccount andDisplayView:(UIView*)displayView andVideoConference:(NSString*)conferenceId andPwd:(NSString*)conferencePwd andPort:(NSInteger)port
{
    if (voipAccount.length<=0)
    {
        return -1;
    }
    else if(displayView == nil)
    {
        return -2;
    }
    else if (conferenceId.length <= 0)
    {
        return -3;
    }
    else if (conferencePwd == nil)
    {
        conferencePwd = @"";
    }

    NSString *confId = conferenceId;
    if (conferenceId.length >= 30)
    {
        confId = [conferenceId substringWithRange:NSMakeRange(14, 16)];
    }
    else if(conferenceId.length >= 22)
    {
        confId = [conferenceId substringWithRange:NSMakeRange(14, 8)];
    }
    NSLog(@"requestConferenceMemberVideoWithAccount port=%d",port);
    return requestMemberVideo(confId.UTF8String, conferencePwd.UTF8String, voipAccount.UTF8String, displayView, port);
}

/*! @function
 ********************************************************************************
 函数名   : stopVideoWithAccount
 功能     : 视频会议中请求某一远端视频
 参数     : [IN]  conferenceNo	  :  所在会议号.
 [IN]  conferencePasswd : 所在会议密码.
 [IN]  remoteSipNo     :  请求远端用户的sip号.
 [IN]  videoWindow     :  当成功请求时，展示该成员的窗口.
 返回值   : 成功 0 失败 -1(voipAccount为NULL) -2(conferenceId为NULL)
 *******************************************************************************/

- (NSInteger)cancelConferenceMemberVideoWithAccount:(NSString*)voipAccount andVideoConference:(NSString*)conferenceId andPwd:(NSString*)conferencePwd
{
    if (voipAccount.length<=0)
    {
        return -1;
    }
    else if (conferenceId.length <= 0)
    {
        return -2;
    }
    else if (conferencePwd == nil)
    {
        conferencePwd = @"";
    }

    NSString *confId = conferenceId;
    if (conferenceId.length >= 30)
    {
        confId = [conferenceId substringWithRange:NSMakeRange(14, 16)];
    }
    else if(conferenceId.length >= 22)
    {
        confId = [conferenceId substringWithRange:NSMakeRange(14, 8)];
    }
    return stopMemberVideo(confId.UTF8String, conferencePwd.UTF8String, voipAccount.UTF8String);
}


//请求发言（主持人模式）
- (void)requestSpeakInVideoConferences:(NSString*)conferenceId OfAppId:(NSString*)appId
{
}

//允许发言（主持人模式）
- (void)allowedVoip:(NSString*)voip toSpeakInVideoConferences:(NSString*)conferenceId OfAppId:(NSString*)appId
{
}

//发布视频
-(void)publishVideoInVideoConference:(NSString*)conferenceId ofAppId:(NSString*)appId
{
    [self.ccpRestService publishVideoInMultiVideoConference:conferenceId ofAppId:appId];
}

//取消发布视频
-(void)unpublishVideoInVideoConference:(NSString*)conferenceId ofAppId:(NSString*)appId
{
    [self.ccpRestService unpublishVideoInMultiVideoConference:conferenceId ofAppId:appId];
}
#pragma mark - private
- (NSString *)createDigID
{
    //时间＋６位数字
    static int seedNum = 0;
    //保证是6位数字
    if(seedNum >= 1000000)
        seedNum = 0;
    seedNum++;

    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyyMMddHHmmss"];

    NSString *currentDateStr = [dateFormatter stringFromDate:[NSDate date]];
    [dateFormatter release];

    NSString *digid = [NSString stringWithFormat:@"%@%06d", currentDateStr, seedNum];
    return digid;
}

- (VoipCall *)getVoipCallByDigID:(NSString *)digid
{
    for (NSString *callid in voipCallDict) {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if( [digid compare:call.digID] == NSOrderedSame)
            return call;
    }
    return nil;
}

//播放来电铃声
- (void)playRing
{
    [self playSound:playRingName];
}

//播放呼叫回铃声
- (void)playRingback:(NSString *)callid
{
    [self onLogInfo:[NSString stringWithFormat:@"playRingback callid=%@", callid]];

    VoipCall *call = [voipCallDict valueForKey:callid];
    if(call && call.callStatus == ECallAlerting)
    {
        [self playSound:playRingbackName];
    }
}

- (void)playRingBusy
{
    [self playSound:playRingBusyName];
}

//停止铃声
- (void)stopRing:(BOOL)isCallOver
{
    [self.player stop];
    if (isCallOver)
    {
        self.isCallAndRing = NO;
    }

    NSLog(@"stopRing start");
    NSArray *keyArr = [voipCallDict allKeys];
    for (NSString *callid in keyArr)
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call.callStatus == ECallFailedBusy)
        {
            if([self delegate]
               && [[self delegate] respondsToSelector:@selector(onCallReleased:)])
            {
                call.callStatus = ECallEnd;
                [[self delegate] onCallReleased:call.digID];
            }
            NSLog(@"stopRing removeobject");
            [voipCallDict removeObjectForKey:callid];
        }
    }
    NSLog(@"stopRing end");
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
    if (isVoiceMsgPlaying)
    {
        if (self.delegate && [self.delegate respondsToSelector:@selector(onFinishedPlaying)])
        {
            [self.delegate onFinishedPlaying];
        }
        isVoiceMsgPlaying = NO;
        [self enableLoudsSpeaker:NO];
        return;
    }
    for (NSString *callid in voipCallDict)
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if(call.callStatus == ECallFailedBusy)
        {
            [self onCallReleased:callid];
            return;
        }
    }
}

- (void)playSound:(NSString *)ringName
{
    if (ringName.length > 0)
    {
        NSURL *fileURL = [[NSURL alloc] initFileURLWithPath:ringName];
        if (fileURL)
        {
            AVAudioPlayer *newPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error: nil];
            newPlayer.delegate = self;
            [fileURL release];
            self.player = newPlayer;
            [newPlayer release];
            [self.player prepareToPlay];

            if( [ringName compare:playRingBusyName] == NSOrderedSame )
                self.player.numberOfLoops = 1;
            else
                self.player.numberOfLoops = -1;    // Loop playback until invoke stop method

            [self.player play];

            self.isCallAndRing = YES;
        }
    }
}

- (NSString*)getResourceBundlePath:(NSString *)filename
{
    NSString * bundlePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent: CCPSDK_BUNDLE_NAME];

    NSBundle * libBundle = [NSBundle bundleWithPath: bundlePath];
    if ( libBundle && filename )
    {
        NSString * s=[[libBundle resourcePath] stringByAppendingPathComponent:filename];
        return s;
    }
    return nil ;
}

- (void)appDidEnterBackgroundFun:(NSNotification *)notification
{
    [self stopRing:NO];
    [self onLogInfo:[NSString stringWithFormat:@"appWillResignActiveFun"]];
    [[UIApplication sharedApplication] setKeepAliveTimeout:600 handler:^{
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]startingKeepAlive account=%@", self.account]];

        NetworkStatus status = [hostReach currentReachabilityStatus];
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]KeepAlive NetStatus=%d status=%d isRegisted=%d", NetStatus, status, isRegisted]];

        bool isKeepAlive = false;

        if(status == NOReachable ||  status == NotReachable)
        {
            setNetworkType(NETWORK_STATUS_NONE, false, false);
        }
        else
        {
            if(NetStatus != status)
            {
                if(status == ReachableViaWiFi)
                {
                    setNetworkType(NETWORK_STATUS_WIFI, true, true);
                }
                else if(status == ReachableViaWWAN)
                {
                    NSNumber* type  = [self NetworkTypeFromStatusBar];
                    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]reachabilityChanged NetworkType=%d", [type intValue]]];

                    if ([type intValue] == BarNetworkNOReachable) {
                        setNetworkType(NETWORK_STATUS_NONE, false, false);
                    }
                    else if([type intValue] == BarNetwork2G)
                    {
                        setNetworkType(NETWORK_STATUS_GPRS, true, true);
                    }
                    else if( [type intValue] == BarNetwork3G)
                    {
                        setNetworkType(NETWORK_STATUS_3G, true, true);
                    }
                    else if( [type intValue] == BarNetworkWifi1)
                    {
                        setNetworkType(NETWORK_STATUS_3G, true, true);
                    }
                    else if ([type intValue] == BarNetworkWifi2)
                    {
                        setNetworkType(NETWORK_STATUS_WIFI, true, true);
                    }
                }
            }
            else
            {
                isKeepAlive = true;
                sendKeepAlive();
            }
        }


        int i = 0;
        while (i < 4)
        {
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]Handle Loops i=%d", i]];
            sleep(1);
            sendKeepAlive();
            i++;
        }

        NetStatus = status;
        [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]startingKeepAlive end"]];
    }];
}

- (NSInteger)resetAudio:(BOOL)flag
{
    int ret = resetAudioDevice();//重置音频设备
    return ret;
}

- (void)appWillEnterForegroundFun:(NSNotification *)notification
{
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]appWillEnterForegroundFun"]];
    sendKeepAlive();
}

- (void)reachabilityChangedWLAN:(NSNotification *)notify
{
    NSNumber* type  = [self NetworkTypeFromStatusBar];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]reachabilityChangedWLAN NetworkType=%d", type.intValue]];
    if(NetStatus == ReachableViaWWAN)
    {
        {
            ENETWORK_STATUS enStatus = NETWORK_STATUS_NONE;
            if ([type intValue] == BarNetwork2G)
            {
                enStatus = NETWORK_STATUS_GPRS;
            }
            else if ([type intValue] == BarNetwork3G)
            {
                enStatus = NETWORK_STATUS_3G;
            }
            else if ([type intValue] == BarNetworkWifi1)
            {
                enStatus = NETWORK_STATUS_3G;
            }
            else if ([type intValue] == BarNetworkWifi2)
            {
                enStatus = NETWORK_STATUS_WIFI;
            }
            setNetworkType(enStatus, true, false);
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onReachbilityChanged:)])
            {
                [[g_CCPCallService delegate] onReachbilityChanged:enStatus];
            }
        }
    }
}

- (void)reachabilityChanged:(NSNotification *)note
{
	TReachability* curReach = [note object];
    NetworkStatus status = [curReach currentReachabilityStatus];

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]reachabilityChanged NetStatus=%d status=%d isRegisted=%d", NetStatus, status, isRegisted]];

    NSNumber* type  = [self NetworkTypeFromStatusBar];
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]reachabilityChanged NetworkType=%d", type.intValue]];
    if (status == NotReachable)
    {
        setNetworkType(NETWORK_STATUS_NONE, false, false);
    }
    else if (NetStatus != status)
    {
        if(status == ReachableViaWiFi)
        {
            setNetworkType(NETWORK_STATUS_WIFI, true, true);
            [self.ccpRestService getNetGroupId];
        }
        else if(status == ReachableViaWWAN)
        {
            //网络切换后，状态栏可能不会立即改变，需要延时获取
            setNetworkType(NETWORK_STATUS_3G, true, true);
            [self performSelector:@selector(reachabilityChangedWLAN:) withObject:nil afterDelay:1.0];
        }
    }
    NetStatus = status;
    if (NetStatus != ReachableViaWWAN)
    {
        ENETWORK_STATUS enStatus = NETWORK_STATUS_NONE;
        if (NetStatus == ReachableViaWiFi)
        {
            enStatus = NETWORK_STATUS_WIFI;
        }
        else if (NetStatus == NotReachable || NetStatus == NOReachable)
        {
            enStatus = NETWORK_STATUS_NONE;
        }
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onReachbilityChanged:)])
        {
            [[g_CCPCallService delegate] onReachbilityChanged:enStatus];
        }
    }
}

//-(void)reConnectToCcpServer
//{
//    if(self.account && self.password &&  (self.clpss || (self.ccp_server_ip && self.ccp_serverport)))
//    {
//        if ([self.clpss length]>0)
//        {
//            connectToCCPWithXML([self.clpss UTF8String],[self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
//        }
//        else
//        {
//            connectToCCP([self.ccp_server_ip cStringUsingEncoding:NSUTF8StringEncoding], self.ccp_serverport, [self.account cStringUsingEncoding:NSUTF8StringEncoding], [self.password cStringUsingEncoding:NSUTF8StringEncoding],[self.strCapability cStringUsingEncoding:NSUTF8StringEncoding]);
//        }
//    }
//}

// 上传多媒体IM文件结果
-(void)sendMediaMsgFinished:(NSNotification *)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    id data = [notification.userInfo objectForKey:KEY_USERINFO_SEND_MEDIAMSG_DATA];

    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] sendMediaMsgFinished statuscode=%d",status.reason]];

    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onSendInstanceMessageWithReason:andMsg:)])
    {
        [[g_CCPCallService delegate] onSendInstanceMessageWithReason:status andMsg:data];
    }
}


// 下载语音文件回调
- (void)downloadMediaMsgsFinished:(NSNotification *)notification
{
    DownloadInfo* info = [notification.userInfo objectForKey:KEY_USERINFO_DOWNLOADFILEINFO];
    NSString * fileName = info.fileName;
    NSString* strRetCode = [notification.userInfo objectForKey:KEY_RESPONSE_STATUSCODE];
    NSString* strRetMsg = [notification.userInfo objectForKey:KEY_RESPONSE_STATUSCODE];
    NSInteger retCode = strRetCode.integerValue;
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] downloadVoiceMsgsFinished strRetCode is %@",strRetCode]];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onDownloadAttachedWithReason:andFileName:)])
    {
        CloopenReason *reason = [[CloopenReason alloc] init];
        reason.reason = retCode;
        reason.msg = strRetMsg;
        [[g_CCPCallService delegate] onDownloadAttachedWithReason:reason andFileName:fileName];
        [reason release];
    }
}

- (void)confirmDownloadAttachedFinished:(NSNotification*)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onConfirmInstanceMessageWithReason:)])
    {
        [[g_CCPCallService delegate] onConfirmInstanceMessageWithReason:status];
    }
}

- (void)checkVoipOfflineMsgFinished:(NSNotification*)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onCheckVoipOfflineMsgsWithReason:andReceiveMsgs:)])
    {
        [[g_CCPCallService delegate] onCheckVoipOfflineMsgsWithReason:status andReceiveMsgs:[notification.userInfo objectForKey:KEY_RESPONSE_USERDATA]];
    }
}

- (void)getVoipOfflineMsgFinished:(NSNotification*)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onGetVoipOfflineMsgWithReason:andMsg:)])
    {
        [[g_CCPCallService delegate] onGetVoipOfflineMsgWithReason:status andMsg:[notification.userInfo objectForKey:KEY_RESPONSE_USERDATA]];
    }
}

- (void)confirmOfflineMsgFinished:(NSNotification*)notification
{
    CloopenReason * status = [self requestStatuscode:notification];
    if(g_CCPCallService && [g_CCPCallService delegate]
       && [[g_CCPCallService delegate] respondsToSelector:@selector(onConfirmOfflineMsgWithReason:)])
    {
        [[g_CCPCallService delegate] onConfirmOfflineMsgWithReason:status];
    }
}

- (void)checkReachabilty
{
//    if (hostReach)
//    {
//        [hostReach stopNotifier];
//        [[NSNotificationCenter defaultCenter] removeObserver:self name:kReachabilityChangedNotification object:nil];
//    }
//
//    [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(reachabilityChanged:) name: kReachabilityChangedNotification object: nil];
//	hostReach = [[TReachability reachabilityWithHostName: @"www.apple.com"] retain];
//    [hostReach startNotifier];
}

- (void)enterVoipCallFlow:(BOOL)status
{
    [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService]enterVoipCallFlow=%d", status]];
    //人脸接近检测开启
    [UIDevice currentDevice].proximityMonitoringEnabled = status;
    //自动锁屏功能关闭
    [UIApplication sharedApplication].idleTimerDisabled= status;
}

- (NSNumber*)NetworkTypeFromStatusBar
{
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7) {
        if (self.networkInfo.currentRadioAccessTechnology)
        {
            int type = [self ios7NetworkType:self.networkInfo.currentRadioAccessTechnology];
            if (type == 0)
            {
                return [NSNumber numberWithInt:BarNetwork2G];
            }
            else if(type == 1)
            {
                return [NSNumber numberWithInt:BarNetwork3G];
            }
            else if(type == 2)
            {
                return [NSNumber numberWithInt:BarNetworkWifi1];
            }
        }
        else
           return [NSNumber numberWithInt:BarNetworkNOReachable];
    }
    else
    {
        UIApplication *app=[UIApplication sharedApplication];
        UIStatusBar *statusBar=[app valueForKey:@"statusBar"];
        UIStatusBarForegroundView *foregroundView=[statusBar valueForKey:@"foregroundView"];
        NSArray *subviews=[foregroundView subviews];
        UIStatusBarDataNetworkItemView *dataNetworkItemView = nil;

        for(id subview in subviews)
        {
            NSLog(@"subview = %@", subview);
            if([subview isKindOfClass:[NSClassFromString(@"UIStatusBarDataNetworkItemView")class]])
            {
                dataNetworkItemView = subview;
                break;
            }
        }

        NSNumber *networkStatus = [dataNetworkItemView valueForKey:@"dataNetworkType"];
        if (nil == networkStatus) {
            return [NSNumber numberWithInt:0];
        }
        return networkStatus;
    }
    return [NSNumber numberWithInt:0];
}

#pragma mark - AVAudioSessionDelegate
- (void)beginInterruption
{
    [self onLogInfo:@"[CCPCallService]beginInterruption"];

    if (self.delegate && [self.delegate respondsToSelector:@selector(onAudioBeginInterruption)])
    {
        [self.delegate onAudioBeginInterruption];
    }
}

- (void)endInterruption
{
    [self onLogInfo:@"[CCPCallService]endInterruption"];
    if (self.delegate && [self.delegate respondsToSelector:@selector(onAudioEndInterruption)])
    {
        [self.delegate onAudioEndInterruption];
    }
}

#ifdef _custom_method_version_

/**
 * 设置语音SRTP加密属性
 * @param enabel    YES设置srtp加密；NO不设置srtp加密。该值为NO时，isUserMode、cryptType、key等参数均忽略。
 * @param isUserMode  YES用户模式，NO标准模式。用户模式时加解密使用本地设置的key，非用户模式时，标准srtp模式。
 * @param cryptType 0:AES_256_SHA1_80, 1:AES_256_SHA1_32
 * @param key : 加解密秘钥（长度46个字节）
 */
-(NSInteger) setSrtpEnable:(BOOL)enabel  andUserMode:(BOOL)isUserMode andCryptType:(NSInteger)cryptType andKey:(NSString*) key
{
    int type = AES_256_SHA1_80;
    if (cryptType == 1)
    {
        type = AES_256_SHA1_32;
    }

    if (key == nil)
    {
        key = @"";
    }

    int ret = setSrtpEnabled(TRUE, enabel, isUserMode, type, [key cStringUsingEncoding:NSUTF8StringEncoding]);
    return ret;
}

#endif

//打开静音检测功能，有效减少语音通话的带宽 默认关闭
//-(void) setDtxEnabel:(BOOL)enable
//{
//    setDtxEnabled(enable);
//}

-(NSInteger)setAudioConfigEnabledWithType:(EAudioType) type andEnabled:(BOOL) enabled andMode:(NSInteger) mode
{
   return setAudioConfigEnabled(type, enabled, mode);
}

-(AudioConfig*)getAudioConfigEnabelWithType:(EAudioType) type
{
    bool enabel;
    int mode;
    int ret = getAudioConfigEnabled(type, &enabel, &mode);
    if (ret == 0)
    {
        AudioConfig *config = [[AudioConfig alloc] init];
        config.audioType = type;
        config.enable = enabel;
        config.audioMode = mode;
        return [config autorelease];
    }
    return nil;
}

-(void)setVideoBitRates:(NSInteger)bitrates{
    setVideoBitRates(bitrates);
}

-(NSInteger)setSilkRate:(NSInteger)rate
{
    return setSilkRate(rate);
}
/**
 * 保存Rtp数据到文件，只能在通话过程中调用，如果没有调用stopRtpDump，通话结束后底层会自动调用
 * @param callid    回话ID
 * @param mediaType 媒体类型， 0：音频 1：视频
 * @param fileName  文件名
 * @paramdirection  需要保存RTP的方向，0：接收 1：发送
 * @return     成功 0 失败 -1
 */
-(NSInteger) startRtpDump:(NSString*)callid andMediaType:(NSInteger) mediaType andFileName:(NSString*)fileName andDirection:(NSInteger) direction;
{
   return  startRtpDump([callid UTF8String], mediaType, [fileName UTF8String], direction);
}

/**
 * 停止保存RTP数据，只能在通话过程中调用。
 * @param   callid :  回话ID
 * @param   mediaType: 媒体类型， 0：音频 1：视频
 * @param   direction: 需要保存RTP的方向，0：接收 1：发送
 * @return  成功 0 失败 -1
 */
-(NSInteger) stopRtpDump:(NSString*)callid andMediaType:(NSInteger) mediaType  andDirection:(NSInteger) direction;
{
    return  stopRtpDump([callid UTF8String], mediaType, direction);
}

-(void)sendChunkedFileWithVoiceMsg:(id) data andServerIP:(NSString *)serverIp withServerPort:(NSString *)serverPort byUrl:(NSString *)url
{
    IMAttachedMsg* msg = data;
    Consumer* myConsumer = [sendTask objectForKey:msg.fileUrl];
    if (myConsumer)
    {
        [myConsumer sendChunkedFileWithVoiceMsg:(id) data andServerIP:serverIp withServerPort:serverPort byUrl:url];
    }
}

- (void) cancelVoiceRecording
{
    if (self.recordTimer)
    {
        [self.recordTimer invalidate];
        self.recordTimer = nil;
    }
    if (self.decibelsTimer)
    {
        [self.decibelsTimer invalidate];
        self.decibelsTimer = nil;
    }
    if (recorder->IsRunning())
        recorder->StopRecord(YES);
}

// 下载语音文件
- (void)downloadIMChunkedMsgs:(NSArray*)urlList
{
    for (DownloadInfo *downInfo in urlList)
    {
        CLOPHTTPRequest *request = [[[CLOPHTTPRequest alloc] initWithURL:[NSURL URLWithString:downInfo.fileUrl]] autorelease];
        request.requestType = ERequestType_DownloadIMChunkedFile;
        request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:downInfo, KEY_USERINFO_DOWNLOADFILEINFO, nil];
        [request setDownloadDestinationPath:downInfo.fileName];
        [request setDelegate:self];
        [request startAsynchronous];
    }
}

#pragma mark - 多媒体IM

//发送录音
- (NSString*) startVoiceRecordingWithReceiver:(NSString*) receiver andPath:(NSString*) path andChunked:(BOOL) chunked andUserdata:(NSString*)userdata
{
    char buffer[64] = {'\0'};
    int ret = getUniqueID(buffer, 64);
    if (ret < 0)
    {
        return nil;
    }
    NSString *sendMsgid = [NSString stringWithCString:buffer encoding:NSUTF8StringEncoding];
    if (chunked)
    {
        [self startRecordingChunkedWithFileName:path andReceiver:receiver andChunked:chunked andUserdata:userdata andSendMsgId:sendMsgid];
    }
    else
    {
        [self startRecordingWithFileName:path];
    }
    return sendMsgid;
}

//发送多媒体信息及文本信息
- (NSString*) sendInstanceMessage:(NSString*) receiver andText:(NSString*) text andAttached:(NSString*) attached andUserdata:(NSString*)userdata
{
    if (text.length>0 && attached.length <= 0)
    {
        NSString* strText = text;
        NSString* strUserdata = nil;

        if ([text lengthOfBytesUsingEncoding:NSUTF8StringEncoding] > 2000)
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onSendInstanceMessageWithReason:andMsg:)])
            {
                CloopenReason *reason = [[CloopenReason alloc] init];
                reason.reason = ELocalReason_ErrorTextTooLong;
                reason.msg = @"发送文本过长";

                IMTextMsg * textmsg = [[IMTextMsg alloc] init];
                textmsg.message = text;
                textmsg.status = @"-1";
                [[g_CCPCallService delegate] onSendInstanceMessageWithReason:reason andMsg:textmsg];
                [textmsg release];
                [reason release];
                return nil;
            }
        }
        else
            self.messageContent = text;

        if (userdata.length > 255)
        {
            strUserdata = [userdata substringToIndex:255];
        }
        else
           strUserdata = userdata;

        self.messageReceiver = receiver;

        if (!isRegisted)
        {
            return nil;
            CloopenReason *clpresaon = [[CloopenReason alloc] init];
            clpresaon.reason = ELocalReason_NotRegisted;
            [[self delegate] onSendInstanceMessageWithReason:clpresaon andMsg:nil];
            [clpresaon release];
        }
        if (strUserdata == nil) {
            strUserdata = @"userdata";
        }
        const char* msgId = sendTextMessage([receiver cStringUsingEncoding:NSUTF8StringEncoding], [strText cStringUsingEncoding:NSUTF8StringEncoding], [strUserdata cStringUsingEncoding:NSUTF8StringEncoding]);
        return [NSString stringWithCString:msgId encoding:NSUTF8StringEncoding];
    }
    else if(attached.length > 0)
    {
        char buffer[64] = {'\0'};
        int ret = getUniqueID(buffer, 64);
        if (ret < 0)
        {
            return nil;
        }
        NSString* strUserdata = nil;
        if (userdata.length > 255)
        {
            strUserdata = [userdata substringToIndex:139];
        }
        else
            strUserdata = userdata;
        if (strUserdata == nil) {
            strUserdata = @"userdata";
        }
        NSString *sendMsgid = [NSString stringWithCString:buffer encoding:NSUTF8StringEncoding];
        [self.ccpRestService sendMediaMsgWithFileName:attached andReceiver:receiver andChunked:NO andUserdata:strUserdata andSendMsgId:sendMsgid];
        return sendMsgid;
    }
    return nil;
}

//确认已经下载
- (void) confirmInstanceMessageWithMsgId:(NSArray*) msgIds
{
    [self.ccpRestService confirmInstanceMessageWithMsgId:msgIds];
}


- (void) checkVoipOfflineMsgs
{
    [self.ccpRestService checkVoipOfflineMsgWithTimestampStr:self.lastTimeStr];
}

- (void) getVoipOfflineMsgWithSender:(NSString*) sender andTimestamp:(NSString*) strTimestamp andLimit:(int)limit
{
    [self.ccpRestService getVoipOfflineMsgWithSender:sender andTimestampStr:strTimestamp andLimit:limit];
}

- (void) confirmOfflineMsgWithMsgIds:(NSArray*) msgIds
{
    [self.ccpRestService confirmOfflineMsgWithMsgIds:msgIds];
}

//下载
- (void) downloadAttached:(NSArray*)urlList
{
    NSMutableArray * chunkedArray = [[NSMutableArray alloc] init];
    NSMutableArray * normalArray = [[NSMutableArray alloc] init];
    for (DownloadInfo*  down in urlList) {
        if (down.fileUrl.length == 0) {
            continue;
        }
        if (down.isChunked) {
            [chunkedArray addObject:down];
        }
        else
            [normalArray addObject:down];
    }
    if ([normalArray count] > 0)
    {
        [self.ccpRestService downloadAttachmentFiles:urlList];
    }
    if ([chunkedArray count] > 0)
    {
        [self downloadIMChunkedMsgs:chunkedArray];
    }
    [chunkedArray release];
    [normalArray release];
}

#pragma mark - CLOPHTTPRequestDelegate

-(void)request:(CLOPHTTPRequest *)request  didReceiveData:(NSData *)data
{
    if (!(request.requestType == ERequestType_DownloadIMChunkedFile)) {
        return;
    }
    //int ilen = 0;
    FILE * _AmrFile = fopen((const char *)[request.downloadDestinationPath UTF8String], "ab+");
    if (0!= _AmrFile) {
        //ilen = fwrite([data bytes],1,[data length],_AmrFile);
        fwrite([data bytes],1,[data length],_AmrFile);
    }
    if (_AmrFile)
    {
        fclose(_AmrFile);
    }
    if ([data length] >= 11) {
        NSRange range;
        range.length = 11;
        range.location = [data length] - 11;
        NSData* subData = [data subdataWithRange:range];
        NSString *aString = [[NSString alloc] initWithData:subData encoding:NSUTF8StringEncoding];
        if ([aString isEqualToString: @"#!HISUNSTOP"])
        {
            [request cancel];
            NSString *fileName = nil;
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] downloadVoiceMsgsFinished HISUNSTOP"]];
            DownloadInfo* info = [request.userInfo objectForKey:KEY_USERINFO_DOWNLOADFILEINFO];
            fileName = info.fileName;

            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onDownloadAttachedWithReason:andFileName:)])
            {

                [[g_CCPCallService delegate] onDownloadAttachedWithReason:0 andFileName:fileName];
            }
        }
        else if([aString isEqualToString: @"!HISUNERROR"])
        {
            [request cancel];
            NSString *fileName = nil;
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] downloadVoiceMsgsFinished HISUNERROR"]];
            DownloadInfo* info = [request.userInfo objectForKey:KEY_USERINFO_DOWNLOADFILEINFO];
            fileName = info.fileName;

            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onDownloadAttachedWithReason:andFileName:)])
            {
                CloopenReason *reason = [[CloopenReason alloc] init];
                reason.reason = ELocalReason_UploadCancel;
                reason.msg = @"对方取消发送录音";
                [[g_CCPCallService delegate] onDownloadAttachedWithReason:reason andFileName:fileName];
                [reason release];
            }
        }
        [aString release];
    }
}

//- (void)requestFinished:(CLOPHTTPRequest *)request
//{
//
//}

- (void)requestFailed:(CLOPHTTPRequest *)request
{
    int reason = 0;
    switch (request.requestType)
    {
        case ERequestType_DownloadIMChunkedFile:
        {
            reason = ELocalReason_DwonLoadIMChunkedFailed;
            NSError *error = [request error];
            NSString *fileName = nil;
            [self onLogInfo:[NSString stringWithFormat:@"[CCPCallService] downloadIMChunked requestFailed %@", error]];
            DownloadInfo* info = [request.userInfo objectForKey:KEY_USERINFO_DOWNLOADFILEINFO];
            fileName = info.fileName;
            if (error.code == 4)
            {
                reason = EIMFileDoesNotExist;
            }
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onDownloadAttachedWithReason:andFileName:)])
            {
                CloopenReason * creason = [[CloopenReason alloc] init];
                creason.reason = reason;
                [[g_CCPCallService delegate] onDownloadAttachedWithReason:creason andFileName:fileName];
                [creason release];
            }
        }
            break;
        default:
            break;
    }

}

- (void)handleCallEvent
{
	// Instantiate CTTelephonyNetworkInfo and CTCallCenter objects.
	CTCallCenter *tmpCallCenter = [[CTCallCenter alloc] init];
    self.callCenter = tmpCallCenter;
    [tmpCallCenter release];
	// Define callEventHandler block inline
	self.callCenter.callEventHandler = ^(CTCall* inCTCall)
    {
        if ( inCTCall.callState == CTCallStateDialing )  //The call state, before connection is established, when the user initiates the call.
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onReceiveEvents:)])
            {

                [[g_CCPCallService delegate] onReceiveEvents:SYSCallComing];
            }
        }
        else if ( inCTCall.callState == CTCallStateIncoming )   //来电
        {
            if(g_CCPCallService && [g_CCPCallService delegate]
               && [[g_CCPCallService delegate] respondsToSelector:@selector(onReceiveEvents:)])
            {

                [[g_CCPCallService delegate] onReceiveEvents:SYSCallComing];
            }
        }
    };
}


-(void) checkBattery
{
    if ([[UIDevice currentDevice] batteryLevel] * 100 <=10)
    {
        if(g_CCPCallService && [g_CCPCallService delegate]
           && [[g_CCPCallService delegate] respondsToSelector:@selector(onReceiveEvents:)])
        {

            [[g_CCPCallService delegate] onReceiveEvents:BatteryLower];
        }
    }
}

-(NSString*)getLIBVersion
{
    return [NSString stringWithUTF8String:getVersion()];
}

- (NetworkStatistic*)getNetworkStatisticWithCallId:(NSString*) digid
{
    long long duration = 0;
    long long send_total_sim = 0;
    long long recv_total_sim = 0;
    long long send_total_wifi = 0;
    long long recv_total_wifi = 0;

    NSString* call_ID = nil;
    if (digid.length<=0)
    {
        return nil;
    }

    if (runningType == ERunningType_Interphone
        || runningType == ERunningType_ChatRoom
        || runningType == ERunningType_VideoConference)
    {
        call_ID = self.interphoneCallId;
    }
    else
    {
        for (NSString *callid in voipCallDict)
        {
            VoipCall *call = [voipCallDict valueForKey:callid];
            if( [digid compare:call.digID] == NSOrderedSame)
                call_ID = call.callID;
        }
        if (call_ID.length <=0)
        {
            return nil;
        }
    }

    //    CCPAPI int getNetworkStatistic(const char *callid, long long *duration, long long *send_total_sim, long long *recv_total_sim, long long *send_total_wifi, long long *recv_total_wifi);
    int ret = getNetworkStatistic([call_ID UTF8String], &duration, &send_total_sim,&recv_total_sim, &send_total_wifi, &recv_total_wifi);
    if(ret==0)
    {
        NetworkStatistic* networkStatistic = [[[NetworkStatistic alloc] init] autorelease];
        networkStatistic.duration = duration;
        networkStatistic.txBytes_sim = send_total_sim;
        networkStatistic.rxBytes_sim = recv_total_sim;
        networkStatistic.txBytes_wifi = send_total_wifi;
        networkStatistic.rxBytes_wifi = recv_total_wifi;
        return networkStatistic;
    }
    return nil;
}

/*! @function
 ********************************************************************************
 函数名   : setStunServer
 功能     : 设置Stun Server。通过这个server获取公网地址。
 参数     :  [IN] server : 服务器地址
 参数     :  [IN] port   : 服务端口，默认3478
 返回值   :  成功 0 失败 -1
 *********************************************************************************/
-(NSInteger) setStunServerWithServer:(NSString*) server andPort:(NSInteger) port
{
    int iPort = 0;
    if (port < 0)
    {
        iPort = 3478;
    }
    else
        iPort = port;

    return setStunServer([server UTF8String],iPort);
}

/*! @function
 ********************************************************************************
 函数名   : setFirewallPolicy
 功能     : 设置防火墙类型。
 参数     :  [IN] policy : 防火墙类型。
 typedef enum _CCPClientFirewallPolicy {
 SerphonePolicyNoFirewall = 0,
 SerphonePolicyUseIce
 } CCPClientFirewallPolicy;
 返回值   :  成功 0 失败 -1
 *********************************************************************************/

-(NSInteger) setFirewallPolicy:(APPFirewallPolicy) Policy
{
    CCPClientFirewallPolicy FirewallPolicy;

    if (Policy == phonePolicyNoFirewall)
    {
        FirewallPolicy = SerphonePolicyNoFirewall;
    }
    else
        FirewallPolicy = SerphonePolicyUseIce;

    return setFirewallPolicy(FirewallPolicy);
}

/*! @function
 ********************************************************************************
 函数名   : seRateAfterP2PSucceed
 功能     : 视频通话且P2P成功后，为了取得更好的画质，可以将码流设置的高一些。默认330。
 参数     :  [IN] rate : 码流，默认330。
 返回值   :  成功 0 失败 -1
 *********************************************************************************/
-(NSInteger) setRateAfterP2PSucceed:(NSInteger) rate;
{
    return seRateAfterP2PSucceed(rate);
}

#ifdef ShieldMosaic
/*! @function
 ********************************************************************************
 函数名   : setShieldMosaic
 功能     : 设置是否屏蔽视频解码过程中的马赛克。默认不屏蔽。
 参数     :  [IN] flag : TRUE 屏蔽；FALSE 不屏蔽。
 返回值   :  成功 0 失败 -1
 *********************************************************************************/
-(NSInteger) setShieldMosaic:(BOOL) flag
{
    return setShieldMosaic(flag);
}
#else
-(NSInteger) setShieldMosaic:(BOOL) flag
{
    return 0;
}
#endif


- (NSString*)getDeviceVersion
{
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString *deviceString = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];

    if ([deviceString isEqualToString:@"iPhone1,1"])    return @"iPhone 1G";
    if ([deviceString isEqualToString:@"iPhone1,2"])    return @"iPhone 3G";
    if ([deviceString isEqualToString:@"iPhone2,1"])    return @"iPhone 3GS";
    if ([deviceString isEqualToString:@"iPhone3,1"])    return @"iPhone 4";
    if ([deviceString isEqualToString:@"iPhone4,1"])    return @"iPhone 4S";
    if ([deviceString isEqualToString:@"iPhone3,2"])    return @"Verizon iPhone 4";
    if ([deviceString isEqualToString:@"iPod1,1"])      return @"iPod Touch 1G";
    if ([deviceString isEqualToString:@"iPod2,1"])      return @"iPod Touch 2G";
    if ([deviceString isEqualToString:@"iPod3,1"])      return @"iPod Touch 3G";
    if ([deviceString isEqualToString:@"iPod4,1"])      return @"iPod Touch 4G";
    if ([deviceString isEqualToString:@"iPad1,1"])      return @"iPad";
    if ([deviceString isEqualToString:@"iPad2,1"])      return @"iPad 2 (WiFi)";
    if ([deviceString isEqualToString:@"iPad2,2"])      return @"iPad 2 (GSM)";
    if ([deviceString isEqualToString:@"iPad2,3"])      return @"iPad 2 (CDMA)";
    if ([deviceString isEqualToString:@"i386"])         return @"Simulator";
    if ([deviceString isEqualToString:@"x86_64"])       return @"Simulator";
    return deviceString;
}
-(NSInteger)disConnectToCCP
{
    int ret = disConnectToCCP();
    if (ret == 0)
    {
        isRegisted = false;
    }
    return ret;
}


- (UIImage *)getVideoSnapshotType:(NSInteger)type WithCallid:(NSString*)digid
{
    unsigned char *buf = nullptr;
    unsigned int size = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    int ret = 0;

    if (digid.length==0)
    {
        return nil;
    }

    NSString* call_ID = nil;
    for (NSString *callid in voipCallDict)
    {
        VoipCall *call = [voipCallDict valueForKey:callid];
        if( [digid compare:call.digID] == NSOrderedSame)
            call_ID = call.callID;
    }

    if (call_ID.length ==0)
    {
        return nil;
    }

    if (type == 0)
    {
        ret = getRemoteVideoSnapshot([call_ID UTF8String], &buf, &size, &width, &height);
    }
    else
    {
        ret = getLocalVideoSnapshot([call_ID UTF8String], &buf, &size, &width, &height);
    }

    if (ret == 0)
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
        CFDataRef data2 = CFDataCreate(kCFAllocatorDefault, buf,  width*height*3);//CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, buf, width*height*3,kCFAllocatorNull);
        CGDataProviderRef provider = CGDataProviderCreateWithCFData(data2);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGImageRef cgImage = CGImageCreate(width,
                                           height,
                                           8,
                                           24,
                                           width*3,
                                           colorSpace,
                                           bitmapInfo,
                                           provider,
                                           NULL,
                                           NO,
                                           kCGRenderingIntentDefault);
        CGColorSpaceRelease(colorSpace);
        UIImage *image = [[UIImage alloc] initWithCGImage:cgImage];
        CGImageRelease(cgImage);
        CGDataProviderRelease(provider);
        CFRelease(data2);
        [pool release];
        free(buf);
        return [image autorelease];
    }
    return nil;
}
- (UIImage *)getRemoteVideoSnapshotWithCallid:(NSString*)callid
{
    return [self getVideoSnapshotType:0 WithCallid:callid];
}
- (UIImage *)getLocalVideoSnapshotWithCallid:(NSString *)callid
{
    return [self getVideoSnapshotType:1 WithCallid:callid];
}


- (void) editTestNumWithOldPhoneNumber:(NSString*) oldPhoneNumber andNewPhoneNumber:(NSString*) newPhoneNumber andServerIP:(NSString*) server_IP andServerPort:(NSInteger) server_Port andServerVersion :(NSString*) serverVersion andMainAccount:(NSString*) mainAccount andMainToken:(NSString*) mainToken
{
    [self.ccpRestService editTestNumWithOldPhoneNumber:oldPhoneNumber andNewPhoneNumber:newPhoneNumber andServerIP:server_IP andServerPort:server_Port andServerVersion:serverVersion andMainAccount:mainAccount andMainToken:mainToken];
}

-(void) setTraceFlag:(BOOL) flag
{
    setTraceFlag(flag);
}

- (NSString*)getSDKDate
{
    return [NSString stringWithFormat:@"%@" ,kSDKDate];
}

- (NSString*)getSDKVersion
{
    return [NSString stringWithFormat:@"%@" ,kSDKVersion];
}

-(StatisticsInfo*)getCallStatistics
{
    MediaStatisticsInfo callStatisticsInfo;
    int ret = getCallStatistics(0,&callStatisticsInfo);
    if (ret == 0)
    {
        StatisticsInfo* retStatisticsInfo = [[StatisticsInfo alloc] autorelease];
        retStatisticsInfo.rlFractionLost =  callStatisticsInfo.fractionLost;
        retStatisticsInfo.rlCumulativeLost =  callStatisticsInfo.cumulativeLost;
        retStatisticsInfo.rlExtendedMax =  callStatisticsInfo.extendedMax;
        retStatisticsInfo.rlJitterSamples =  callStatisticsInfo.jitterSamples;
        retStatisticsInfo.rlRttMs =  callStatisticsInfo.rttMs;
        retStatisticsInfo.rlBytesSent =  callStatisticsInfo.bytesSent;
        retStatisticsInfo.rlPacketsSent =  callStatisticsInfo.packetsSent;
        retStatisticsInfo.rlBytesReceived =  callStatisticsInfo.bytesReceived;
        retStatisticsInfo.rlPacketsReceived =  callStatisticsInfo.packetsReceived;
        return retStatisticsInfo;
    }
    return nil;
}


#ifdef _custom_method_version_

/**
 * 开始通话录音
 * @param callid 电话id
 * @param filename 录音文件名，保存为wav格式
 */
- (NSInteger) startRecordVoiceCallWithCallid:(NSString*) callid andFilename:(NSString*) filename
{
    NSString* call_ID = nil;
    int callType = 0;
    for (NSString *in_callid in voipCallDict)
    {
        VoipCall *call = [voipCallDict valueForKey:in_callid];
        if( [callid compare:call.digID] == NSOrderedSame)
        {
            call_ID = call.callID;
            callType = call.callType;
            break;
        }
    }
    if([call_ID length] >0)
        return startRecordVoice([call_ID UTF8String],[filename UTF8String]);
    else
        return -1;
}

/**
 * 停止通话录音
 * @param callid 电话id
 */
- (NSInteger) stopRecordVoiceCallWithCallid:(NSString*) callid
{
    NSString* call_ID = nil;
    int callType = 0;
    for (NSString *in_callid in voipCallDict)
    {
        VoipCall *call = [voipCallDict valueForKey:in_callid];
        if( [callid compare:call.digID] == NSOrderedSame)
        {
            call_ID = call.callID;
            callType = call.callType;
            break;
        }
    }
    if([call_ID length] >0)
        return stopRecordVoice([call_ID UTF8String]);
    else
        return -1;
}

/**
 *判断voip用户是否在线，同步返回结果，最长阻塞3秒
 * @param  voipAccount  voip账号
 * @return 1在线，0不在线
 **/
-(NSInteger) checkUserOnlineWithAccount:(NSString*) voipAccount
{
    return checkUserOnline([voipAccount UTF8String]);
}

/**设置允许上层处理音频数据
 * @param    callid	   当前呼叫的唯一标识.
 * @param    flag      YES 允许上层处理； NO 不允许上层处理。
 * @return   成功 0 失败-1
 */
- (NSInteger)setProcessDataEnabledWithCallid:(NSString*) callid andFlag:(BOOL) flag;
{
    NSString* call_ID = nil;
    int callType = 0;
    for (NSString *in_callid in voipCallDict)
    {
        VoipCall *call = [voipCallDict valueForKey:in_callid];
        if( [callid compare:call.digID] == NSOrderedSame)
        {
            call_ID = call.callID;
            callType = call.callType;
            break;
        }
    }
    if([call_ID length] >0)
        return setProcessDataEnabled([call_ID UTF8String],flag);
    else
        return -1;
}

/**设置上报上层音频数据
 * @param    callid	   当前呼叫的唯一标识.
 * @param    flag      YES 允许上报； NO 不允许上报。
 * @return   成功 0 失败-1
 */
- (NSInteger) setProcessOriginalDataEnabledWithCallid:(NSString*) callid andFlag:(BOOL) flag
{
    NSString* call_ID = nil;
    int callType = 0;
    for (NSString *in_callid in voipCallDict)
    {
        VoipCall *call = [voipCallDict valueForKey:in_callid];
        if( [callid compare:call.digID] == NSOrderedSame)
        {
            call_ID = call.callID;
            callType = call.callType;
            break;
        }
    }
    if([call_ID length] >0)
        return setProcessOriginalDataEnabled([call_ID UTF8String],flag);
    else
        return -1;
}

/**
 * 私有云校验接口
 * @param    companyID	   企业ID
 * @param    restAddr      软交换地址
 * @param    isNativeCheck 是否本地校验
 * @return   成功0;  -1:companyID过长（最大199）; -2:restAdd过长（99）; -3:companyID为空; -4:restAdd:为空
 */
- (NSInteger) setPrivateCloudWithCompanyID:(NSString*)companyID andRestAddr:(NSString*)restAddr andNativeCheck:(BOOL)isNativeCheck
{
    if (companyID.length <= 0)
    {
        return -3;
    }
    else if(restAddr == nil)
    {
        return -4;
    }

    if (isNativeCheck && restAddr.length <=0 )
    {
        return -4;
    }

    return setPrivateCloud([companyID UTF8String], [restAddr UTF8String], isNativeCheck);
}


//设置私有云标记
- (void)setPrivateCloud:(NSString*) flag
{
    if (ccpRestService)
    {
        ccpRestService.PrivateCloudFlag = flag;
    }
}

//设置私有云验证
- (void)setValidate:(NSString*)data
{
    self.ccpValidate = data;
}

//设置私有云服务器版本
-(void)setServerVersion:(NSString*)ver
{
    if (ccpRestService)
    {
        ccpRestService.ServerVerSion = ver;
    }
}
#endif

- (void)setOpusFec:(BOOL)enable
{
    EnableOpusFEC(enable);
}

- (int)setLoss:(int)loss
{
    return SetOpusPacketLossRate(loss);
}
@end

