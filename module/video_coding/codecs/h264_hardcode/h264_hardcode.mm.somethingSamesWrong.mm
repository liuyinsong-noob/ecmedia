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
        
        InProgressFrameEncode(int64_t rtp,
                              int64_t r_time)
        : rtp_timestamp(rtp),
        reference_time(r_time) {}
    };
    
    H264VideoToolboxEncoder* H264VideoToolboxEncoder::Create() {
        return new H264VideoToolboxEncoder();
    }
    
    
    H264VideoToolboxEncoder::H264VideoToolboxEncoder() : encoded_image_(),
    encoded_complete_callback_(NULL),
    inited_(false),
    picture_id_(0),
    periodicKeyFrames_(true),
    framenum_(0),
    bitrate(256000),
    fps(15),
    mode(0),
    generate_keyframe(false),
    num_of_cores_(0){
        
    }
    
    H264VideoToolboxEncoder::~H264VideoToolboxEncoder() {};
    
    int H264VideoToolboxEncoder::Release()
    { //Release buffers and encoder
        if (compression_session_) {
            VTCompressionSessionInvalidate(compression_session_);
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
        ConfigureCompressionSession();
        
        encoded_image_._size =  CalcBufferSize(cloopenwebrtc::kI420, inst->width, inst->height);
        encoded_image_._buffer = new uint8_t[encoded_image_._size];
        encoded_image_._length = 0;
        encoded_image_._completeFrame = false;
        
        // random start 16 bits is enough.
        picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;
        inited_ = true;
        
        
        // Force 420v so that clients can easily use these buffers as GPU textures.
        const int format[] = {
            kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange};
        
        // Keep these attachment settings in-sync with those in ConfigureSession().
        CFTypeRef attachments_keys[] = {kCVImageBufferColorPrimariesKey,
            kCVImageBufferTransferFunctionKey,
            kCVImageBufferYCbCrMatrixKey};
        CFTypeRef attachments_values[] = {kCVImageBufferColorPrimaries_ITU_R_709_2,
            kCVImageBufferTransferFunction_ITU_R_709_2,
            kCVImageBufferYCbCrMatrix_ITU_R_709_2};
        CFTypeRef buffer_attributes_keys[] = {kCVPixelBufferPixelFormatTypeKey,
            kCVBufferPropagatedAttachmentsKey};
        
        
        CFTypeRef buffer_attributes_values[] = {
            ArrayWithIntegers(format, 1),
            DictionaryWithKeysAndValues(attachments_keys, attachments_values, 3)
            };
        const CFDictionaryRef buffer_attributes =
        DictionaryWithKeysAndValues(
                                                   buffer_attributes_keys, buffer_attributes_values,
                                                   2);
        for (auto& v : buffer_attributes_values)
            CFRelease(v);
        
        OSStatus status = VTCompressionSessionCreate(
                                                     kCFAllocatorDefault, inst->width, inst->height,
                                                     kCMVideoCodecType_H264, NULL, buffer_attributes,
                                                     nullptr /* compressedDataAllocator */,
                                                     &H264VideoToolboxEncoder::CompressionCallback,
                                                     reinterpret_cast<void*>(this), &compression_session_);
        
        ConfigureCompressionSession();
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    void H264VideoToolboxEncoder::ConfigureCompressionSession()
    {
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Main_AutoLevel);
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse);
        int value = 240;
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_MaxKeyFrameInterval, CFNumberCreate(nullptr, kCFNumberSInt32Type, &value));
        
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration, CFNumberCreate(nullptr, kCFNumberSInt32Type, &value));
        
        value = (codec_.minBitrate + codec_.maxBitrate)/2;
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_AverageBitRate, CFNumberCreate(nullptr, kCFNumberSInt32Type, &value));
        value = codec_.maxFramerate;
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ExpectedFrameRate, CFNumberCreate(nullptr, kCFNumberSInt32Type, &value));
        
        value = 1300;
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_MaxH264SliceBytes, CFNumberCreate(nullptr, kCFNumberSInt32Type, &value));
//        kVTCompressionPropertyKey_MaxH264SliceBytes
//        // Keep these attachment settings in-sync with those in Initialize().
//        session_property_setter.Set(
//                                    videotoolbox_glue_->kVTCompressionPropertyKey_ColorPrimaries(),
//                                    kCVImageBufferColorPrimaries_ITU_R_709_2);
        
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_ColorPrimaries, kCVImageBufferColorPrimaries_ITU_R_709_2);
        
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_TransferFunction, kCVImageBufferTransferFunction_ITU_R_709_2);
        
        VTSessionSetProperty(compression_session_, kVTCompressionPropertyKey_YCbCrMatrix, kCVImageBufferYCbCrMatrix_ITU_R_709_2);
//        if (video_config_.max_number_of_video_buffers_used > 0) {
//            session_property_setter.Set(
//                                        videotoolbox_glue_->kVTCompressionPropertyKey_MaxFrameDelayCount(),
//                                        video_config_.max_number_of_video_buffers_used);
//        }
        
        
    }
    
    int H264VideoToolboxEncoder::Encode(const I420VideoFrame& input_image,
                            const CodecSpecificInfo* codec_specific_info,
                            const std::vector<VideoFrameType>* frame_types)
    {
        VideoFrameType frameType=kDeltaFrame;
        
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
            //		x264_param_t curparms;
            //		x264_encoder_parameters(encoder_, &curparms);
            //		curparms.i_width = input_image.width();
            //		curparms.i_height = input_image.height();
            //		x264_encoder_reconfig(encoder_, &curparms);
            codec_.width = input_image.width();
            codec_.height = input_image.height();
            frameType = kKeyFrame;
            
            InitEncode(&codec_, num_of_cores_, 30000);
            
        }
        
        
        OSType cv_format = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
        CVPixelBufferRef pixel_buffer;
        //    sean for test only video
        //    frameType = kKeyFrame;
        
        
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
        
        
        void* plane_ptrs[2];
        size_t plane_widths[2] = {480, 240};
        size_t plane_heights[2] = {640, 320};
        size_t plane_bytes_per_row[2] = {640, 320};
        plane_ptrs[0] = (void*)input_image.buffer(PlaneType(0));
        plane_ptrs[1] = (void*)cbcr;
        
        
        
        void* descriptor = calloc(1, std::max(sizeof(CVPlanarPixelBufferInfo_YCbCrPlanar), sizeof(CVPlanarPixelBufferInfo_YCbCrBiPlanar)));
        size_t nbytes = input_image.width()*input_image.height()*5/4;
        
        CVReturn result = CVPixelBufferCreateWithPlanarBytes(
                                                             kCFAllocatorDefault,
                                                             input_image.width(),
                                                             input_image.height(),
                                                             cv_format,
                                                             descriptor,
                                                             nbytes,
                                                             2,
                                                             plane_ptrs,
                                                             plane_widths,
                                                             plane_heights,
                                                             plane_bytes_per_row,
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             &pixel_buffer);
        if (result != kCVReturnSuccess) {
            printf(" CVPixelBufferCreateWithPlanarBytes failed: %d", result);
        }
        
        
        // Convert the frame timestamp to CMTime.
        auto timestamp_cm = CMTimeMake(input_image.render_time_ms(), USEC_PER_SEC);
        
        // Convert render time, in ms, to RTP timestamp.
        const int kMsToRtpTimestamp = 90;
        const uint64_t time_stamp =
        kMsToRtpTimestamp *
        static_cast<uint64_t>(input_image.render_time_ms());
        
        // Wrap information we'll need after the frame is encoded in a heap object.
        // We'll get the pointer back from the VideoToolbox completion callback.
       InProgressFrameEncode request(time_stamp, input_image.render_time_ms());
        
        // Build a suitable frame properties dictionary for keyframes.
        CFDictionaryRef frame_props;
        encode_next_frame_as_keyframe_ = frameType;
        if (encode_next_frame_as_keyframe_) {
            frame_props = DictionaryWithKeyValue(kVTEncodeFrameOptionKey_ForceKeyFrame, kCFBooleanTrue);
            encode_next_frame_as_keyframe_ = false;
        }
//        CVPixelBufferCreateWithPlanarBytes
        
        CMTime dur = CMTimeMake(1, fps);
        // Submit the frame to the compression session. The function returns as soon
        // as the frame has been enqueued.
         VTEncodeInfoFlags flags;
        OSStatus status = VTCompressionSessionEncodeFrame(compression_session_, pixel_buffer, timestamp_cm, CMTimeMake(1, fps), frame_props, &request, &flags);
        if (status != noErr) {
            printf(" VTCompressionSessionEncodeFrame failed: \n", status);
            return false;
        }
        
        return true;
        
//        picture_id_ = (framenum_ + 1) & 0x7FFF;  // prepare next
//        int ret = x264_encoder_encode(encoder_,&xnals,&num_nals,&xpic,&oxpic);
//        
//        if (ret > 0)
//        {
//            
//            CodecSpecificInfo codec;
//            CodecSpecificInfoH264 *h264Info = &(codec.codecSpecific.H264);
//            RTPFragmentationHeader fragment;
//            bool bNonReference = CopyEncodedImage(fragment, xnals, num_nals, &oxpic, input_image);
//            codec.codecType = kVideoCodecH264;
//            h264Info->pictureId = framenum_;
//            h264Info->nonReference = bNonReference;
//            encoded_complete_callback_->Encoded(encoded_image_, &codec, &fragment);
//            
//            framenum_++;
//        }
//        else{
//            WEBRTC_TRACE(cloopenwebrtc::kTraceError,
//                         cloopenwebrtc::kTraceVideoCoding,
//                         0,
//                         "x264_encoder_encode() error=%d.", ret);
//        }
        
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    void H264VideoToolboxEncoder::CompressionCallback(void* encoder_opaque,
                             void* request_opaque,
                             OSStatus status,
                             VTEncodeInfoFlags info,
                             CMSampleBufferRef sbuf)
    {
        
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