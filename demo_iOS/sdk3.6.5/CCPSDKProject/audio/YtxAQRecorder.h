//
//  YtxAQRecorder.h
//  testApp
//
//  Created by  ruitechen on 12-3-29.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <AudioToolbox/AudioToolbox.h>
#include <Foundation/Foundation.h>
#include <libkern/OSAtomic.h>
#include <CoreGraphics/CoreGraphics.h>
#include "CAStreamBasicDescription.h"
#include "CAXException.h"
#import  "Consumer.h"
#import "../CCPClient_Internal.h"

#define kNumberRecordBuffers    3

class YtxAQRecorder
{
public:
    YtxAQRecorder();
    ~YtxAQRecorder();
    UInt32                      GetNumberChannels() const   { return mRecordFormat.NumberChannels(); }
    CFStringRef                 GetFileName() const         { return mFileName; }
    AudioQueueRef               Queue() const               { return mQueue; }
    CAStreamBasicDescription    DataFormat() const          { return mRecordFormat; }
    
    void            StartRecord(CFStringRef inRecordFile);
    void            StartRealTimeRecord(CFStringRef inRecordFile);
    void            StopRecord(BOOL flag);
    Boolean         IsRunning() const           { return mIsRunning; }
    Float32         getPeakPower();
    void EncodeBuffer(short* buf,int len);
    void setChannelNumbers(NSArray * v);
    void  setConsumer(Consumer *myConsumer);
    UInt64          startTime;
    CGFloat mFileDuration;
    
    FILE *_AmrFile;
    int* _destate;
    AudioQueueLevelMeterState       *_chan_lvls;
    NSArray                         *_channelNumbers;
    long                            bufferCount;
    Consumer                        *consumer;
    Boolean                         isRealTime;
    unsigned char                   soundBuf[50000]= {0};
    long long                       iSendCount;
    long long                       iSoundCount;
    Boolean                         readySend;
private:
    CFStringRef                 mFileName;
    AudioQueueRef               mQueue;
    AudioQueueBufferRef         mBuffers[kNumberRecordBuffers];
    AudioFileID                 mRecordFile;
    SInt64                      mRecordPacket; // current packet number in record file
    CAStreamBasicDescription    mRecordFormat;
    Boolean                     mIsRunning;
    
    void            CopyEncoderCookieToFile();
    void            SetupAudioFormat(UInt32 inFormatID);
    int             ComputeRecordBufferSize(const AudioStreamBasicDescription *format, float seconds);
    
    static void MyInputBufferHandler(   void *                              inUserData,
                                     AudioQueueRef                       inAQ,
                                     AudioQueueBufferRef                 inBuffer,
                                     const AudioTimeStamp *              inStartTime,
                                     UInt32                              inNumPackets,
                                     const AudioStreamPacketDescription* inPacketDesc);
};


