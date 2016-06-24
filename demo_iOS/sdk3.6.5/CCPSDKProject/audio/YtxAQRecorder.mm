//
//  YtxAQRecorder.m
//  testApp
//
//  Created by  ruitechen on 12-3-29.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include "YtxAQRecorder.h"

extern "C"
{
#include "interf_enc.h"
}

#define SAMPLES_PER_SECOND 8000.0f
const float kBufferDurationSeconds = 0.5;

// Determine the size, in bytes, of a buffer necessary to represent the supplied number
// of seconds of audio data.
int YtxAQRecorder::ComputeRecordBufferSize(const AudioStreamBasicDescription *format, float seconds)
{
    int packets, frames, bytes = 0;
    try {
        frames = (int)ceil(seconds * format->mSampleRate);
        
        if (format->mBytesPerFrame > 0)
            bytes = frames * format->mBytesPerFrame;
        else {
            UInt32 maxPacketSize;
            if (format->mBytesPerPacket > 0)
                maxPacketSize = format->mBytesPerPacket;    // constant packet size
            else {
                UInt32 propertySize = sizeof(maxPacketSize);
                XThrowIfError(AudioQueueGetProperty(mQueue, kAudioQueueProperty_MaximumOutputPacketSize, &maxPacketSize,&propertySize), "couldn't get queue's maximum output packet size");
                
            }
            if (format->mFramesPerPacket > 0)
                packets = frames / format->mFramesPerPacket;
            else
                packets = frames;   // worst-case scenario: 1 frame in a packet
            if (packets == 0)       // sanity check
                packets = 1;
            bytes = packets * maxPacketSize;
        }
    }
    catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        return 0;
    }
    return bytes;
}

// ____________________________________________________________________________________
// AudioQueue callback function, called when an input buffers has been filled.
void YtxAQRecorder::MyInputBufferHandler(  void *                              inUserData,
                                      AudioQueueRef                       inAQ,
                                      AudioQueueBufferRef                 inBuffer,
                                      const AudioTimeStamp *              inStartTime,
                                      UInt32                              inNumPackets,
                                      const AudioStreamPacketDescription* inPacketDesc)
{
    YtxAQRecorder *aqr = (YtxAQRecorder *)inUserData;
    try {
        if (inNumPackets > 0) {
            // // write packets to file
            //            XThrowIfError(AudioFileWritePackets(aqr->mRecordFile, FALSE, inBuffer->mAudioDataByteSize,
            // inPacketDesc, aqr->mRecordPacket, &inNumPackets, inBuffer->mAudioData),
            //   "AudioFileWritePackets failed");
            //            aqr->mRecordPacket += inNumPackets;
            
            for (int i =0; i < inBuffer->mAudioDataByteSize ;i+=160*2) {
                short * pPacket = (short *)(((unsigned char*)(inBuffer->mAudioData))+i);
                
                const short par = 2;
                for (int j=0; j<160; j++) {
                    if (pPacket[j]<(0x7FFF/par)&&pPacket[j]>(-0x7FFF/par)) {
                        if (pPacket[j] > 0x7FFF/2) {
                            pPacket[j] = 0x7FFF-1;
                        }else if (pPacket[j] < -0x7FFF/2) {
                            pPacket[j] = -0x7FFF+1;
                        }else{
                            pPacket[j] = pPacket[j]*par;
                        }
                    }
                }
                
                aqr->EncodeBuffer(pPacket,320);
            }
            
            aqr->mRecordPacket += inNumPackets;
            //int duration   = (int)(aqr->mRecordPacket * (aqr->mRecordFormat).mFramesPerPacket) %  (int)((aqr->mRecordFormat).mSampleRate) >= 0.5 ? 1 : 0;
            aqr->mFileDuration = (aqr->mRecordPacket * (aqr->mRecordFormat).mFramesPerPacket) / (aqr->mRecordFormat).mSampleRate;// + duration;
            
        }
        
        // if we're not stopping, re-enqueue the buffe so that it gets filled again
        if (aqr->IsRunning())
            XThrowIfError(AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL), "AudioQueueEnqueueBuffer failed");
    } catch (CAXException e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
}

YtxAQRecorder::YtxAQRecorder()
{
    mIsRunning = false;
    mRecordPacket = 0;
    _destate = (int*) Encoder_Interface_init(0);
    mFileName=NULL;
    //tony add
    _channelNumbers = [[NSArray alloc] initWithObjects:[NSNumber numberWithInt:0], nil];
    _chan_lvls = (AudioQueueLevelMeterState*)malloc(sizeof(AudioQueueLevelMeterState) * [_channelNumbers count]);
}

YtxAQRecorder::~YtxAQRecorder()
{
    AudioQueueDispose(mQueue, TRUE);
    AudioFileClose(mRecordFile);
    if (mFileName) CFRelease(mFileName);
    
    Encoder_Interface_exit((void*)_destate);
    _destate = 0;
    
    if (_AmrFile)
    {
        fclose(_AmrFile);
    }
    [_channelNumbers release];
    free(_chan_lvls);
}

// ____________________________________________________________________________________
// Copy a queue's encoder's magic cookie to an audio file.
void YtxAQRecorder::CopyEncoderCookieToFile()
{
    UInt32 propertySize;
    // get the magic cookie, if any, from the converter
    OSStatus err = AudioQueueGetPropertySize(mQueue, kAudioQueueProperty_MagicCookie, &propertySize);
    
    // we can get a noErr result and also a propertySize == 0
    // -- if the file format does support magic cookies, but this file doesn't have one.
    if (err == noErr && propertySize > 0) {
        Byte *magicCookie = new Byte[propertySize];
        UInt32 magicCookieSize;
        XThrowIfError(AudioQueueGetProperty(mQueue, kAudioQueueProperty_MagicCookie, magicCookie, &propertySize), "get audio converter's magic cookie");
        magicCookieSize = propertySize; // the converter lies and tell us the wrong size
        
        // now set the magic cookie on the output file
        UInt32 willEatTheCookie = false;
        // the converter wants to give us one; will the file take it?
        err = AudioFileGetPropertyInfo(mRecordFile, kAudioFilePropertyMagicCookieData, NULL, &willEatTheCookie);
        if (err == noErr && willEatTheCookie) {
            err = AudioFileSetProperty(mRecordFile, kAudioFilePropertyMagicCookieData, magicCookieSize, magicCookie);
            XThrowIfError(err, "set audio file's magic cookie");
        }
        delete[] magicCookie;
    }
}

void YtxAQRecorder::SetupAudioFormat(UInt32 inFormatID)
{
    memset(&mRecordFormat, 0, sizeof(mRecordFormat));
    mRecordFormat.mFormatID = inFormatID;
    if (inFormatID == kAudioFormatLinearPCM)
    {
        // if we want pcm, default to signed 16-bit little-endian
        mRecordFormat.mSampleRate = SAMPLES_PER_SECOND; // amr 8khz
        mRecordFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        mRecordFormat.mBitsPerChannel = 16;
        mRecordFormat.mChannelsPerFrame = 1;
        mRecordFormat.mFramesPerPacket = 1;
        
        mRecordFormat.mBytesPerFrame = (mRecordFormat.mBitsPerChannel/8) * mRecordFormat.mChannelsPerFrame;
        mRecordFormat.mBytesPerPacket =  mRecordFormat.mBytesPerFrame ;
    }
}

void YtxAQRecorder:: setChannelNumbers(NSArray *v)
{
    [v retain];
    [_channelNumbers release];
    _channelNumbers = v;
}

void YtxAQRecorder::StartRealTimeRecord(CFStringRef inRecordFile)
{
    isRealTime = YES;
    StartRecord(inRecordFile);
}

void YtxAQRecorder::StartRecord(CFStringRef inRecordFile)
{
    if (isRealTime)
    {
        bufferCount = 0;
        iSendCount = 0;
        iSoundCount = 0;
        memset(soundBuf, 0, sizeof(soundBuf));
        readySend = NO;
    }
    int i, bufferByteSize;
    try {
        SetupAudioFormat(kAudioFormatLinearPCM);
        
        // create the queue
        XThrowIfError(AudioQueueNewInput(
                                         &mRecordFormat,
                                         MyInputBufferHandler,
                                         this ,
                                         NULL , NULL ,
                                         0 , &mQueue), "AudioQueueNewInput failed");

        mRecordPacket = 0;
        
        bufferByteSize = ComputeRecordBufferSize(&mRecordFormat, kBufferDurationSeconds);   // enough bytes for half a second
        for (i = 0; i < kNumberRecordBuffers; ++i) {
            XThrowIfError(AudioQueueAllocateBuffer(mQueue, bufferByteSize, &mBuffers[i]),
                          "AudioQueueAllocateBuffer failed");
            XThrowIfError(AudioQueueEnqueueBuffer(mQueue, mBuffers[i], 0, NULL),
                          "AudioQueueEnqueueBuffer failed");
        }
        // start the queue
        mIsRunning = true;
        AudioQueueFlush(mQueue);
        
        Float32 gain=.8;
        
        //设置音量
        AudioQueueSetParameter(mQueue, kAudioQueueParam_Volume, gain);
        XThrowIfError(AudioQueueStart(mQueue, NULL), "AudioQueueStart failed");
        
        if(mFileName){
            CFRelease(mFileName);
            mFileName = nil;
        }
        
        
        UInt32 val = 1;
        AudioQueueSetProperty(mQueue, kAudioQueueProperty_EnableLevelMetering, &val, sizeof(UInt32));
        
        if (mRecordFormat.mChannelsPerFrame != [_channelNumbers count])
        {
            NSArray *chan_array;
            if (mRecordFormat.mChannelsPerFrame < 2)
                chan_array = [[NSArray alloc] initWithObjects:[NSNumber numberWithInt:0], nil];
            else
                chan_array = [[NSArray alloc] initWithObjects:[NSNumber numberWithInt:0], [NSNumber numberWithInt:1], nil];
            
            setChannelNumbers(chan_array);
            [chan_array release];
            
            _chan_lvls = (AudioQueueLevelMeterState*)realloc(_chan_lvls, mRecordFormat.mChannelsPerFrame * sizeof(AudioQueueLevelMeterState));
        
        }
        
        mFileName = CFStringCreateCopy(kCFAllocatorDefault, inRecordFile);
        mRecordPacket = 0;
        if (0!=mFileName) {
            _AmrFile = fopen((const char *)[mFileName UTF8String], "wb+");
            XThrowIfError(0 == _AmrFile, "Amr file create failed");
            fwrite("#!AMR\n", 1, strlen("#!AMR\n"), _AmrFile);
        }
        
    }
    catch (CAXException &e) {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    }
    catch (...) {
        fprintf(stderr, "An unknown error occurred\n");
    }   
    
}
Float32 YtxAQRecorder:: getPeakPower()
{
    if (mQueue)
    {
        UInt32 data_sz = sizeof(AudioQueueLevelMeterState) * [_channelNumbers count];
        OSErr status = AudioQueueGetProperty(mQueue, kAudioQueueProperty_CurrentLevelMeterDB, _chan_lvls, &data_sz);
        if (status == noErr)
        {
            // 这里没有去处理多个通道的数据显示,直接就显示最后一个通道的结果了
            // 这里的值就是我们打算用来做为一些触发机制的值了,需要用到的时候直接访问_chan_lvls这个数组
            for (int i=0; i<[_channelNumbers count]; i++)
            {
                NSInteger channelIdx = [(NSNumber *)[_channelNumbers objectAtIndex:i] intValue];
                if (channelIdx < [_channelNumbers count] && channelIdx <= 127)
                {
                    return  _chan_lvls[channelIdx].mPeakPower;
                }
            }
        }
    }
    return 0;
}
void YtxAQRecorder::StopRecord(BOOL flag)
{
    // end recording
    mIsRunning = false;
    XThrowIfError(AudioQueueStop(mQueue, true), "AudioQueueStop failed");   
    // a codec may update its cookie at the end of an encoding session, so reapply it to the file now
    CopyEncoderCookieToFile();
    NSString* str = (NSString*)mFileName;
    if (mFileName)
    {
        CFRelease(mFileName);
        mFileName = NULL;
    }
    if (mQueue) {
        AudioQueueDispose(mQueue, true);
    }
    if (mRecordFile) {
        AudioFileClose(mRecordFile);
    }
    
    if (_AmrFile)
    {
        if (isRealTime)
        {
            if (readySend)
            {
                if (flag)
                {
                    NSDictionary* cancelDict = [NSDictionary dictionaryWithObjectsAndKeys:@"voice",@"type",@"cancel",@"state",str,@"file",nil,@"data", nil];
                    [consumer createProductWithData:cancelDict];
                    remove([str UTF8String]);
                }
                else
                {
                    NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
                    [dict setValue:@"voice" forKey:@"type"];
                    [dict setValue:@"end" forKey:@"state"];
                    [dict setValue:str forKey:@"file"];
                    NSData* data = nil;
                    if (iSoundCount - iSendCount > 0)
                    {
                        data = [NSData dataWithBytes:soundBuf+iSendCount length:iSoundCount - iSendCount];
                    }
                    [dict setValue:data forKey:@"data"];
                    [consumer createProductWithData:dict];
                    [dict release];
                }
            }
            else
            {
                remove([str UTF8String]);
                [consumer sendUploadFailedTimeIsShort];
            }
            bufferCount = 0;
            iSendCount = 0;
            iSoundCount = 0;
            readySend = NO;
        }
        fclose(_AmrFile);
        _AmrFile=nil;
    }
}

void YtxAQRecorder::setConsumer(Consumer *myConsumer){
    consumer = myConsumer;
}

void YtxAQRecorder::EncodeBuffer(short *buf,int len)
{
    if (buf == NULL)
    {
        NSLog(@"%p",buf);
        return;
    }
    unsigned char serialbuf[320]= {0};
    memset(serialbuf, 0, sizeof(serialbuf));
    int FrameLen =0;
    short noiseBuf[3840] = {0};
    noiseSuppression((const void *)buf, noiseBuf);
    FrameLen= Encoder_Interface_Encode(_destate,MR475,noiseBuf,serialbuf,0);
    if (isRealTime)
    {
        bufferCount++;
        if (bufferCount % 50 == 0 && bufferCount > 0)
        {
            NSString* str = (NSString*) mFileName;
            if (bufferCount == 50)
            {
                readySend = YES;
                NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:@"voice",@"type",@"begin",@"state",str,@"file",nil,@"data", nil];
                [consumer createProductWithData:dict];
                [consumer sendRest];
            }
            
            NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
            [dict setValue:@"voice" forKey:@"type"];
            [dict setValue:@"normal" forKey:@"state"];

            [dict setValue:str forKey:@"file"];
            NSMutableData* data = [NSMutableData dataWithBytes:soundBuf+iSendCount length:iSoundCount - iSendCount];
            [dict setValue:data forKey:@"data"];
            [consumer createProductWithData:dict];
            [dict release];
            iSendCount = iSoundCount;
        }
        memcpy(soundBuf+iSoundCount,serialbuf,FrameLen); //
        iSoundCount += FrameLen;
    }

    int ilen = 0;
    if (0!= _AmrFile) {
        ilen = fwrite((unsigned char *)serialbuf,1,FrameLen,_AmrFile);
    }
}