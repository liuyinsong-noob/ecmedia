//
//  AMRPlayer.m
//
//  Created by ruitechen on 12-3-28.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#import "AMRPlayer.h"
#include "interf_dec.h"

const unsigned int PACKETNUM =  25;//20ms * 25 = 0.5s ,每个包25帧,可以播放0.5秒
const float KSECONDSPERBUFFER = 0.2; //每秒播放0.2个缓冲
const unsigned int AMRFRAMELEN = 32; //帧长
const unsigned int PERREADFRAME =  10;//每次读取帧数
static unsigned int gBufferSizeBytes = 0x10000;

@implementation AMRPlayer

@synthesize queue;

// 回调（Callback）函数的实现

static void BufferCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef buffer) {
    AMRPlayer* player = (AMRPlayer*)inUserData;
    [player  audioQueueOutputWithQueue:inAQ queueBuffer:buffer];
}

//初始化方法（为NSObject中定义的初始化方法）

- (id) init {
    //for(int i=0; i<NUM_BUFFERS; i++) {
    //        AudioQueueEnqueueBuffer(queue,buffers[i],0,nil);
    //    }
    
    
    _destate = Decoder_Interface_init();
    
    return self;
}

//- (id) initWithFileName:(char*)fileName {
//
//
// _destate = Decoder_Interface_init();
// [self initFile:fileName];
//    return self;
//}

-(void)initFile:(char*) amrFileName
{
    
#ifdef IF2
    //short block_size[16]={ 12, 13, 15, 17, 18, 20, 25, 30, 5, 0, 0, 0, 0, 0, 0, 0 };
#else
    char magic[8];
    //short block_size[16]={ 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };
#endif
    
    _hasReadSize=0;
    
    _amrFile = fopen(amrFileName, "rb");
    
    
    
    int rCout=fread( magic, sizeof( char ), strlen( "#!AMR\n" ), _amrFile );
    _hasReadSize=rCout;
    if ( strncmp( magic, "#!AMR\n", strlen( "#!AMR\n" ) ) ) {
        
        fclose( _amrFile );
    }
    
    
}

//缓存数据读取方法的实现

- (void) audioQueueOutputWithQueue:(AudioQueueRef)audioQueue
                       queueBuffer:(AudioQueueBufferRef)audioQueueBuffer {
    
    //OSStatus status;
    
    // 读取包数据
    //UInt32  numBytes;
    
    //UInt32  numPackets = numPacketsToRead;
    
    //status = AudioFileReadPackets( audioFile, NO, &numBytes, packetDescs, packetIndex, &numPackets, audioQueueBuffer->mAudioData);
    
    //-----
    short pcmBuf[1600]={0};//KSECONDSPERBUFFER * 160 * 50;
    
    int readAMRFrame = 0;
    const short block_size[16]={ 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };
    char analysis[32]={0};
    
    
    int rCout=0;
    // while (fread(analysis, sizeof (unsigned char), 1, file_analysis ) > 0)
    while (readAMRFrame < PERREADFRAME && (rCout=fread(analysis, sizeof (unsigned char), 1, _amrFile)))
    {
        int dec_mode = (analysis[0] >> 3) & 0x000F;
        
        int read_size = block_size[dec_mode];
        _hasReadSize += rCout;
        rCout=fread(&analysis[1], sizeof (char), read_size, _amrFile);
        
        _hasReadSize += rCout;
        
        Decoder_Interface_Decode(_destate,(unsigned char *)analysis,&pcmBuf[readAMRFrame*160],0);
        readAMRFrame ++;
    }
    
    //NSLog(@"readCount:%d",_hasReadSize);
    
    if (readAMRFrame > 0) {
        audioQueueBuffer ->mAudioDataByteSize = readAMRFrame * 2 * 160;
        audioQueueBuffer ->mPacketDescriptionCount = readAMRFrame*160;
        memcpy(audioQueueBuffer ->mAudioData, pcmBuf, readAMRFrame * 160 *2);
        AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);
        
    }
    //---
    
    // 成功读取时
    //if (numPackets > 0) {
    
    //将缓冲的容量设置为与读取的音频数据一样大小（确保内存空间）
    // audioQueueBuffer->mAudioDataByteSize = numBytes;
    
    // 完成给队列配置缓存的处理
    // status = AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, numPackets, packetDescs);
    
    // 移动包的位置
    // packetIndex += numPackets;
    // }
}

//音频播放方法的实现

-(void) startPlay:(const char*) path
{//CFURLRef
    
    [self initFile:path];
    // UInt32      size, maxPacketSize;
    
    // char        *cookie;
    
    // int         i;
    
    //  OSStatus status;
    
    // 打开音频文件
    //status = AudioFileOpenURL(path, kAudioFileReadPermission, 0, &audioFile);
    //    if (status != noErr) {
    //        // 错误处理
    //        return;
    //    }
    
    // 取得音频数据格式
    //size = sizeof(dataFormat);
    //AudioFileGetProperty(audioFile, kAudioFilePropertyDataFormat,&size, &dataFormat);
    
    //--设置音频数据格式
    memset(&dataFormat, 0, sizeof(dataFormat));
    dataFormat.mFormatID = kAudioFormatLinearPCM;
    dataFormat.mSampleRate = 8000.0;
    dataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    dataFormat.mBitsPerChannel = 16;
    dataFormat.mChannelsPerFrame = 1;
    dataFormat.mFramesPerPacket = 1;
    
    dataFormat.mBytesPerFrame = (dataFormat.mBitsPerChannel/8) * dataFormat.mChannelsPerFrame;
    dataFormat.mBytesPerPacket = dataFormat.mBytesPerFrame ;
    
    //---
    
    // 创建播放用的音频队列(nil:audio队列的间隙线程)
    AudioQueueNewOutput(&dataFormat, BufferCallback,self, nil, nil, 0, &queue);
    
    //==独立线程的模式
    //AudioQueueNewOutput(&dataFormat, BufferCallback, self, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &queue);
    
    
    //计算单位时间包含的包数
    //    if (dataFormat.mBytesPerPacket==0 || dataFormat.mFramesPerPacket==0) {
    //
    //        size = sizeof(maxPacketSize);
    //        AudioFileGetProperty(audioFile,
    //                             kAudioFilePropertyPacketSizeUpperBound, &size, &maxPacketSize);
    //        if (maxPacketSize > gBufferSizeBytes) {
    //            maxPacketSize = gBufferSizeBytes;
    //
    //        }
    //
    //        // 算出单位时间内含有的包数
    //        numPacketsToRead = gBufferSizeBytes / maxPacketSize;
    //        packetDescs = malloc(
    //                             sizeof(AudioStreamPacketDescription) * numPacketsToRead);
    //    } else {
    //        numPacketsToRead = gBufferSizeBytes / dataFormat.mBytesPerPacket;
    //        packetDescs = nil;
    //    }
    
    //    //设置Magic Cookie，参见第二十七章的相关介绍
    //    AudioFileGetPropertyInfo(audioFile,
    //                             kAudioFilePropertyMagicCookieData, &size, nil);
    //    if (size > 0) {
    //        cookie = malloc(sizeof(char) * size);
    //        AudioFileGetProperty(audioFile,
    //                             kAudioFilePropertyMagicCookieData, &size, cookie);
    //        AudioQueueSetProperty(queue,
    //                              kAudioQueueProperty_MagicCookie, cookie, size);
    //        free(cookie);
    //    }
    
    // 创建并分配缓存空间
    packetIndex = 0;
    gBufferSizeBytes = KSECONDSPERBUFFER *  2 * 160 * 50 *2; //MR122 size * 2
    
    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueAllocateBuffer(queue, gBufferSizeBytes, &buffers[i]);//&mBuffers[i]
        
        //读取包数据
        //        if ([self readPacketsIntoBuffer:buffers[i]] == 0) {
        //            break;
        //        }
    }
    
    //设置监听
    //AudioQueueAddPropertyListener(queue, kAudioQueueProperty_IsRunning, isRunningProc, self),
    //static void isRunningProc(void * inUserData,AudioQueueRef queue,AudioQueuePropertyID  inID);
    
    //设置音量
    AudioQueueSetParameter (queue,kAudioQueueParam_Volume,1.0);
    
    //队列处理开始，此后系统会自动调用回调（Callback）函数
    //AudioQueueStart(queue, nil);
    
    [self StartQueue];
}

- (UInt32)readPacketsIntoBuffer:(AudioQueueBufferRef)buffer {
    
    //UInt32      numBytes, numPackets;
    
    // 从文件中接受包数据并保存到缓存(buffer)中
    // numPackets = numPacketsToRead;
    //
    //    AudioFileReadPackets(audioFile, NO, &numBytes, packetDescs,
    //                         packetIndex, &numPackets, buffer->mAudioData);
    //
    //    if (numPackets > 0) {
    //        buffer->mAudioDataByteSize = numBytes;
    //        AudioQueueEnqueueBuffer(queue, buffer,
    //                                (packetDescs ? numPackets : 0), packetDescs);
    //        packetIndex += numPackets;
    //    }
    
    short pcmBuf[1600]={0};; //KSECONDSPERBUFFER * 160 * 50
    
    int readAMRFrame = 0;
    const short block_size[16]={ 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };
    char analysis[32]={0};
    
    int rCout=0;
    while (readAMRFrame < PERREADFRAME && (rCout=fread(analysis, sizeof (unsigned char), 1, _amrFile)))
    {
        
        _hasReadSize += rCout;
        
        int dec_mode = (analysis[0] >> 3) & 0x000F;
        
        int read_size = block_size[dec_mode];
        rCout=fread(&analysis[1], sizeof (char), read_size, _amrFile);
        
        _hasReadSize += rCout;
        
        Decoder_Interface_Decode(_destate,(unsigned char *)analysis,&pcmBuf[readAMRFrame*160],0);
        readAMRFrame ++;
    }
    //NSLog(@"readCount:%d",_hasReadSize);
    
    
    if (readAMRFrame > 0) {
        buffer ->mAudioDataByteSize = readAMRFrame * 2 * 160;
        buffer ->mPacketDescriptionCount = readAMRFrame*160;
        memcpy(buffer ->mAudioData, pcmBuf, readAMRFrame * 160 *2);
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
        
    }
    
    
    return readAMRFrame;
}

- (void)dealloc {
    
    
    AudioQueueDispose(queue, TRUE);
    Decoder_Interface_exit(_destate);
    if (_amrFile) {
        fclose( _amrFile );
    }
    
    [super dealloc];
    
}

-(OSStatus)StartQueue
{   
    
    // prime the queue with some data before starting
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        //读取包数据
        if ([self readPacketsIntoBuffer:buffers[i]] == 0) {
            break;
        }
    }
    return AudioQueueStart(queue, NULL);
}

-(OSStatus)StopQueue
{
    OSStatus result = AudioQueueStop(queue, TRUE);
    if (result) 
        printf("ERROR STOPPING QUEUE!\n");
    
    return result;
}

-(OSStatus)PauseQueue
{
    OSStatus result = AudioQueuePause(queue);
    
    return result;
}

@end