//
//  h264_hardcode.cpp
//  video_coding
//
//  Created by SeanLee on 16/6/23.
//
//

#include "h264_hardcode.h"
#include "video_error_codes.h"
#include "webrtc_libyuv.h"
#include <CoreVideo/CVPixelBuffer.h>
#import <AVKit/AVKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#include "module_common_types.h"
#include <VideoToolbox/VTCompressionProperties.h>
#include "video_codec_interface.h"
char *h264_file = nullptr;
namespace cloopenwebrtc {
    
    CFArrayRef ArrayWithIntegers(const int* v, size_t size) {
        std::vector<CFNumberRef> numbers;
        numbers.reserve(size);
        for (const int* end = v + size; v < end; ++v)
            numbers.push_back(CFNumberCreate(nullptr, kCFNumberSInt32Type, v));
        CFArrayRef array(CFArrayCreate(
                                       kCFAllocatorDefault, reinterpret_cast<const void**>(&numbers[0]),
                                       numbers.size(), &kCFTypeArrayCallBacks));
        for (auto& number : numbers) {
            CFRelease(number);
        }
        return array;
    }
    
    CFDictionaryRef
    DictionaryWithKeysAndValues(CFTypeRef* keys, CFTypeRef* values, size_t size) {
        return CFDictionaryRef(CFDictionaryCreate(
                                                  kCFAllocatorDefault, keys, values, size, &kCFTypeDictionaryKeyCallBacks,
                                                  &kCFTypeDictionaryValueCallBacks));
    }
    
    
    CFDictionaryRef DictionaryWithKeyValue(CFTypeRef key,
                                           CFTypeRef value) {
        CFTypeRef keys[1] = {key};
        CFTypeRef values[1] = {value};
        return DictionaryWithKeysAndValues(keys, values, 1);
    }
    
    struct InProgressFrameEncode {
        const int64_t rtp_timestamp;
        const int64_t reference_time;
        const int64_t timestamp;
        
        InProgressFrameEncode(int64_t rtp,
                              int64_t r_time,
                              int64_t ts)
        : rtp_timestamp(rtp),
        reference_time(r_time),
        timestamp(ts){}
    };
    
    static CVPixelBufferPoolRef CreatePixelBufferPool(int32_t width, int32_t height, OSType pixelFormat, int32_t maxBufferCount)
    {
        CVPixelBufferPoolRef outputPool = NULL;
        
        CFMutableDictionaryRef sourcePixelBufferOptions = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pixelFormat);
        CFDictionaryAddValue(sourcePixelBufferOptions, kCVPixelBufferPixelFormatTypeKey, number);
        CFRelease(number);
        
        number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
        CFDictionaryAddValue(sourcePixelBufferOptions, kCVPixelBufferWidthKey, number);
        CFRelease(number);
        
        number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
        CFDictionaryAddValue(sourcePixelBufferOptions, kCVPixelBufferHeightKey, number);
        CFRelease(number);
        
        CFDictionaryAddValue(sourcePixelBufferOptions, kCVPixelFormatOpenGLESCompatibility, kCFBooleanTrue);
        
        CFDictionaryRef ioSurfaceProps = CFDictionaryCreate(kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (ioSurfaceProps) {
            CFDictionaryAddValue(sourcePixelBufferOptions, kCVPixelBufferIOSurfacePropertiesKey, ioSurfaceProps);
            CFRelease(ioSurfaceProps);
        }
        
        number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &maxBufferCount);
        CFDictionaryRef pixelBufferPoolOptions = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&kCVPixelBufferPoolMinimumBufferCountKey, (const void**)&number, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFRelease(number);
        
        CVPixelBufferPoolCreate(kCFAllocatorDefault, pixelBufferPoolOptions, sourcePixelBufferOptions, &outputPool);
        
        CFRelease(sourcePixelBufferOptions);
        CFRelease(pixelBufferPoolOptions);
        return outputPool;
    }
    
    
    static void PreallocatePixelBuffersInPool( CVPixelBufferPoolRef pool, CFDictionaryRef auxAttributes )
    {
        // Preallocate buffers in the pool, since this is for real-time display/capture
        NSMutableArray *pixelBuffers = [[NSMutableArray alloc] init];
        while ( 1 ) {
            CVPixelBufferRef pixelBuffer = NULL;
            OSStatus err = CVPixelBufferPoolCreatePixelBufferWithAuxAttributes( kCFAllocatorDefault, pool, auxAttributes, &pixelBuffer );
            
            if ( err == kCVReturnWouldExceedAllocationThreshold )
                break;
            assert( err == noErr );
            
            [pixelBuffers addObject:(id)pixelBuffer];
            CFRelease( pixelBuffer );
        }
        [pixelBuffers release];
    }
    
    H264VideoToolboxEncoder* H264VideoToolboxEncoder::Create() {
        return new H264VideoToolboxEncoder();
    }
    
    FILE *H264VideoToolboxEncoder::debug_file_ = NULL;
    int64_t H264VideoToolboxEncoder::size_ = 0;
    EncodedImage H264VideoToolboxEncoder::encoded_image_;
    EncodedImageCallback* H264VideoToolboxEncoder:: encoded_complete_callback_ = NULL;
    VideoCodec H264VideoToolboxEncoder::codec_;
    H264VideoToolboxEncoder::H264VideoToolboxEncoder() :
    inited_(false),
    picture_id_(0),
    periodicKeyFrames_(true),
    framenum_(0),
    bitrate(256000),
    fps(15),
    mode(0),
    generate_keyframe(false),
    num_of_cores_(0){
        if (h264_file) {
            debug_file_ = fopen(h264_file, "wb");
        }
        size_ = 0;
        
        aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    }
    
    H264VideoToolboxEncoder::~H264VideoToolboxEncoder() {
        Release();
        if (debug_file_) {
            fflush(debug_file_);
            fclose(debug_file_);
        }
        
    };
    
    int H264VideoToolboxEncoder::Release()
    { //Release buffers and encoder
        if (compression_session_) {
            VTCompressionSessionInvalidate(compression_session_);
        }
        
        if (pixelBufferPool_) {
            CFRelease(pixelBufferPool_);
            pixelBufferPool_ = NULL;
        }
        if (_bufferPoolAuxAttributes) {
            CFRelease(_bufferPoolAuxAttributes);
            _bufferPoolAuxAttributes = NULL;
        }
        if (encoded_image_._buffer != NULL) {
            delete [] encoded_image_._buffer;
            encoded_image_._buffer = NULL;
        }
    }
    
    int H264VideoToolboxEncoder::SetRates(uint32_t new_bitrate_kbit, uint32_t new_framerate) {
        // VideoToolbox does not seem to support bitrate reconfiguration.
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    
    int H264VideoToolboxEncoder::InitEncode(const VideoCodec* inst,
                                            int32_t number_of_cores,
                                            size_t max_payload_size)
    {
        codec_ = *inst;
        if ((inst == NULL) || (inst->maxFramerate < 1)
            || (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate)
            || (inst->width < 1 || inst->height < 1)
            || (number_of_cores < 1)) {
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
        ConfigureCompressionSession(inst->width, inst->height);
        
        encoded_image_._size =  CalcBufferSize(cloopenwebrtc::kI420, inst->width, inst->height);
        encoded_image_._buffer = new uint8_t[encoded_image_._size];
        encoded_image_._length = 0;
        encoded_image_._completeFrame = false;
        
        // random start 16 bits is enough.
        picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;
        inited_ = true;
        frame_count_ = 0;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    void H264VideoToolboxEncoder::ConfigureCompressionSession(int width, int height)
    {
        //        dispatch_sync(aQueue, ^{
        OSStatus status = VTCompressionSessionCreate(NULL, width, height, kCMVideoCodecType_H264, NULL, NULL, NULL, &H264VideoToolboxEncoder::CompressionCallback, (__bridge void *)(this),  &compression_session_);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_4_1);
        SInt32 bitRate = (codec_.width*codec_.height*15)*2*0.07;
        CFNumberRef ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &bitRate);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_AverageBitRate, ref);
        CFRelease(ref);
        
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_H264EntropyMode, kVTH264EntropyMode_CAVLC);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanTrue);
        int frameRate = 15;
        CFNumberRef frameRateRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &frameRate);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ExpectedFrameRate, frameRateRef);
        CFRelease(frameRateRef);
        
        
//        
//        session_property_setter.Set(
//                                    videotoolbox_glue_->kVTCompressionPropertyKey_AverageBitRate(),
//                                    (video_config_.min_bitrate + video_config_.max_bitrate) / 2);
//        session_property_setter.Set(
//                                    videotoolbox_glue_->kVTCompressionPropertyKey_ExpectedFrameRate(),
//                                    video_config_.max_frame_rate);
//        // Keep these attachment settings in-sync with those in Initialize().
//        session_property_setter.Set(
//                                    videotoolbox_glue_->kVTCompressionPropertyKey_ColorPrimaries(),
//                                    kCVImageBufferColorPrimaries_ITU_R_709_2);
//        session_property_setter.Set(
//                                    videotoolbox_glue_->kVTCompressionPropertyKey_TransferFunction(),
//                                    kCVImageBufferTransferFunction_ITU_R_709_2);
//        session_property_setter.Set(
//                                    videotoolbox_glue_->kVTCompressionPropertyKey_YCbCrMatrix(),
//                                    kCVImageBufferYCbCrMatrix_ITU_R_709_2);
//        if (video_config_.max_number_of_video_buffers_used > 0) {
//            session_property_setter.Set(
//                                        videotoolbox_glue_->kVTCompressionPropertyKey_MaxFrameDelayCount(),
//                                        video_config_.max_number_of_video_buffers_used);
        int expectedFrameRate = codec_.maxBitrate;
        CFNumberRef expectedFrameRateRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &expectedFrameRate);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ExpectedFrameRate, expectedFrameRateRef);
        NSLog(@"sean hehe 111 status %d", status);
        CFRelease(expectedFrameRateRef);
        
        
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ColorPrimaries, kCVImageBufferColorPrimaries_ITU_R_709_2);
        NSLog(@"sean hehe 112 status %d", status);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_TransferFunction, kCVImageBufferTransferFunction_ITU_R_709_2);
        NSLog(@"sean hehe 113 status %d", status);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_YCbCrMatrix, kCVImageBufferYCbCrMatrix_ITU_R_709_2);
        NSLog(@"sean hehe 114 status %d", status);
        
        int maxFrameDelayCount = 1;
        CFNumberRef maxFrameDelayCountRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &maxFrameDelayCount);
        status = VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_MaxFrameDelayCount, maxFrameDelayCountRef);
        NSLog(@"sean1115 %d", status);
        CFRelease(maxFrameDelayCountRef);
        int maxBufferCount = 100;
        pixelBufferPool_ = CreatePixelBufferPool(width, height, kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, maxBufferCount);
        _bufferPoolAuxAttributes = (CFDictionaryRef)[[NSDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithInt:maxBufferCount], (id)kCVPixelBufferPoolAllocationThresholdKey, nil];
        PreallocatePixelBuffersInPool(pixelBufferPool_, _bufferPoolAuxAttributes);
        
        
        VTCompressionSessionPrepareToEncodeFrames(compression_session_);
        //        });
        
        
        
        size_ = 0;
        
        
    }
    
    int H264VideoToolboxEncoder::Encode(const I420VideoFrame& input_image,
                                        const CodecSpecificInfo* codec_specific_info,
                                        const std::vector<VideoFrameType>* frame_types)
    {
        NSLog(@"sean hehe entrance %lld", input_image.timestamp());
        if (0 == input_image.timestamp()) {
            NSLog(@"sean hehe got it");
        }
        VideoFrameType frameType=kDeltaFrame;
        frame_count_++;
        int num_nals=0;
        bool send_key_frame = false;
        
        if (!inited_
            || (encoded_complete_callback_ == NULL) ) {
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (input_image.IsZeroSize()) {
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
        
        for (size_t i = 0; i < frame_types->size();++i) {
            if ((*frame_types)[i] == kKeyFrame) {
                send_key_frame = true;
                frameType = kKeyFrame;
                break;
            }
        }
        
        if(codec_.width != input_image.width() || codec_.height != input_image.height())
        {
            Release();
            
            codec_.width = input_image.width();
            codec_.height = input_image.height();
            frameType = kKeyFrame;
            
            InitEncode(&codec_, num_of_cores_, 30000);
            
        }
        
        
        OSType cv_format = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
        
        NSDictionary *pixelAttributes = @{(id)kCVPixelBufferIOSurfacePropertiesKey : @{}};
        
        //interleave
        
        uint8_t *cbcr;
        int size = input_image.width()*input_image.height()/2;
        cbcr = new uint8_t[size+1];
        int cbcr_cur = 0;
        int cb_cur = 0;
        uint8_t * cb = (uint8_t*)input_image.buffer((PlaneType)1);
        uint8_t * cr = (uint8_t*)input_image.buffer((PlaneType)2);
        for (int cursor = 0; cursor<input_image.height()*input_image.width()/4; cursor++) {
            cbcr[cbcr_cur++] = cb[cb_cur];
            cbcr[cbcr_cur++] = cr[cb_cur++];
        }
        
        
        CVPixelBufferRef pixelBuffer = NULL;
        //        CVReturn result = CVPixelBufferCreate(kCFAllocatorDefault,
        //                                              input_image.width(),
        //                                              input_image.height(),
        //                                              kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
        //                                              (__bridge CFDictionaryRef)(pixelAttributes),
        //                                              &pixelBuffer);
        
        CVReturn err = noErr;
        err = CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(kCFAllocatorDefault, pixelBufferPool_, _bufferPoolAuxAttributes, &pixelBuffer);
        
        if (err) {
            if (kCVReturnWouldExceedAllocationThreshold == err) {
                NSLog(@"Pool is out of buffers, dropping frame");
            }
            else {
                NSLog(@"Error at CVPixelBufferPoolCreatePixelBuffer %d", err);
            }
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        
        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        uint8_t *yDestPlane = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
        memcpy(yDestPlane, input_image.buffer(PlaneType(0)), input_image.width() * input_image.height());
        uint8_t *uvDestPlane = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
        memcpy(uvDestPlane, cbcr, input_image.width()*input_image.height()/2);
        delete [] cbcr;
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        
        
        
        
        // Convert the frame timestamp to CMTime.
        //        auto timestamp_cm = CMTimeMake(input_image.render_time_ms(), USEC_PER_SEC);
        CFDictionaryRef dict_ref = DictionaryWithKeyValue(kVTEncodeFrameOptionKey_ForceKeyFrame, frameType == kKeyFrame?kCFBooleanTrue:kCFBooleanFalse);
        // Wrap information we'll need after the frame is encoded in a heap object.
        // We'll get the pointer back from the VideoToolbox completion callback.
        CMTime timestamp_cm = CMTimeMake(input_image.timestamp(), USEC_PER_SEC);//Apple suggest
        VTEncodeInfoFlags flags;
        
        
        //        CMTime dur = CMTimeMake(1, fps);
        NSLog(@"sean hehe Encode %lld", input_image.timestamp());
        OSStatus status = VTCompressionSessionEncodeFrame(compression_session_, pixelBuffer, timestamp_cm, kCMTimeInvalid, dict_ref, (void*)input_image.timestamp(), &flags);
        CFRelease(dict_ref);
        
        CVPixelBufferRelease(pixelBuffer);
        if (status != noErr) {
            printf(" VTCompressionSessionEncodeFrame failed: %d\n", status);
            return false;
        }
        
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    void H264VideoToolboxEncoder::CompressionCallback(void* encoder_opaque,
                                                      void* request_opaque,
                                                      OSStatus status,
                                                      VTEncodeInfoFlags info,
                                                      CMSampleBufferRef sampleBuffer)
    {
        int64_t ts = (int64_t)request_opaque;
        NSLog(@"sean hehe output status %d %lld\n\n", status, ts);
        
        uint8_t startCode[] = {0,0,0,1};
        
        bool isKeyframe = false;
        
        CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sampleBuffer);
        CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
        //        CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
        //        CMTime dts = CMSampleBufferGetDecodeTimeStamp(sampleBuffer);
        //        CMFormatDescriptionRef Des_ref = CMSampleBufferGetFormatDescription(sampleBuffer);
        //        NSLog(@"staus %d, size %d", status, CMBlockBufferGetDataLength(block));
        
        
        //        NSLog(@"ts %lld",ts);
        if(attachments != NULL) {
            CFDictionaryRef attachment;
            CFBooleanRef dependsOnOthers;
            attachment = (CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
            dependsOnOthers = (CFBooleanRef)CFDictionaryGetValue(attachment, kCMSampleAttachmentKey_DependsOnOthers);
            isKeyframe = (dependsOnOthers == kCFBooleanFalse);
        }
        
        //        isKeyframe = true;
        if(isKeyframe) {
            
            // Send the SPS and PPS.
            CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
            size_t spsSize, ppsSize;
            size_t parmCount;
            const uint8_t* sps, *pps;
            
            CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 0, &sps, &spsSize, &parmCount, nullptr );
            CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 1, &pps, &ppsSize, &parmCount, nullptr );
            
            CodecSpecificInfo codec;
            CodecSpecificInfoH264 *h264Info = &(codec.codecSpecific.H264);
            RTPFragmentationHeader fragment;
            CopyEncodedImage(fragment, 1, ts+1, false, (char *)sps, spsSize, false);
            codec.codecType = kVideoCodecH264;
            h264Info->nonReference = false;
            encoded_complete_callback_->Encoded(encoded_image_, &codec, &fragment);
            CopyEncodedImage(fragment, 1, ts+2, false, (char *)pps, ppsSize, false);
            codec.codecType = kVideoCodecH264;
            h264Info->nonReference = false;
            encoded_complete_callback_->Encoded(encoded_image_, &codec, &fragment);
            
            flockfile(debug_file_);
            fwrite(startCode, 1, 4, debug_file_);
            fwrite(sps, 1, spsSize, debug_file_);
            fwrite(startCode, 1, 4, debug_file_);
            fwrite(pps, 1, ppsSize, debug_file_);
            fflush(debug_file_);
            funlockfile(debug_file_);
        }
        
        
        char* bufferData;
        size_t size, totalsize;
        status = CMBlockBufferGetDataPointer(block, 0, &size, &totalsize, &bufferData);
        
        if (status == noErr) {
            
            size_t bufferOffset = 0;
            static const int AVCCHeaderLength = 4;
            while (bufferOffset < totalsize - AVCCHeaderLength)
            {
                uint32_t NALUnitLength = 0;
                memcpy(&NALUnitLength, bufferData + bufferOffset, AVCCHeaderLength);
                NALUnitLength = CFSwapInt32BigToHost(NALUnitLength);
                //                NSLog(@"sean NALUnitLength %d",NALUnitLength);
                NSData* data = [[NSData alloc] initWithBytes:(bufferData + bufferOffset + AVCCHeaderLength) length:NALUnitLength];
                
                flockfile(debug_file_);
                fwrite(startCode, 1, 4, debug_file_);
                fwrite(bufferData + bufferOffset + AVCCHeaderLength, 1, NALUnitLength, debug_file_);
                fflush(debug_file_);
                funlockfile(debug_file_);
                
                bufferOffset += AVCCHeaderLength + NALUnitLength;
            }
            
            
            
            CodecSpecificInfo codec;
            CodecSpecificInfoH264 *h264Info = &(codec.codecSpecific.H264);
            RTPFragmentationHeader fragment;
            bool bNonReference = CopyEncodedImage(fragment, 1, ts+3, isKeyframe, bufferData, totalsize, true);
            codec.codecType = kVideoCodecH264;
            h264Info->nonReference = bNonReference;
            encoded_complete_callback_->Encoded(encoded_image_, &codec, &fragment);
            
            
            
        }
        
    }
    
    bool H264VideoToolboxEncoder::CopyEncodedImage(RTPFragmentationHeader &fragment, int num_nals, int64_t ts, bool keyFrame, char *buffer, size_t length, bool startcode)
    {
#define NALU_START_PREFIX_LENGTH 4
        
        fragment.VerifyAndAllocateFragmentationHeader(num_nals);
        
        encoded_image_._encodedHeight	= codec_.height;
        encoded_image_._encodedWidth	= codec_.width;
        encoded_image_._frameType		= kKeyFrame ? kKeyFrame : kDeltaFrame;
        encoded_image_._timeStamp		= ts;
        encoded_image_._length          = 0;
        encoded_image_._completeFrame   = true;
        
        for (int i=0; i<num_nals; i++)
        {
            bool b_long_startcode = startcode;
            int offset = b_long_startcode ? NALU_START_PREFIX_LENGTH : 0;
            memcpy(encoded_image_._buffer+encoded_image_._length, buffer+offset, length-offset);
            fragment.fragmentationLength[i] = length-offset;
            fragment.fragmentationOffset[i] = encoded_image_._length;
            //                    fragment.fragmentationPlType[i] = current_nal->i_type;
            encoded_image_._length += length - offset;
        }
        
        return (keyFrame==kKeyFrame) ? false : true;
    }
    
    
    WebRtc_Word32 H264VideoToolboxEncoder::SetPeriodicKeyFrames(bool enable)
    {
        periodicKeyFrames_ = enable;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    WebRtc_Word32 H264VideoToolboxEncoder::SetChannelParameters(WebRtc_UWord32 packetLoss,
                                                                int64_t rtt)
    {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    WebRtc_Word32 H264VideoToolboxEncoder::CodecConfigParameters(WebRtc_UWord8* /*buffer*/, WebRtc_Word32 /*size*/)
    {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxEncoder::RegisterEncodeCompleteCallback(
                                                                EncodedImageCallback* callback) {
        encoded_complete_callback_ = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
}