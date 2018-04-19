/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include "h264_video_toolbox_decoder.h"
#include "h264_sps_parser.h"

#if defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)

#include <memory>

#if defined(WEBRTC_IOS)
#include "RTCUIApplication.h"
#endif
#include "libyuv/convert.h"
#include "checks.h"
#include "logging.h"
#include "corevideo_frame_buffer.h"
#include "h264_video_toolbox_nalu.h"
#include "video_frame.h"
#if DEBUG_H264
char *g_h264file = NULL;
#endif
namespace cloopenwebrtc {
    extern int getVopType( const unsigned char p);
    static const int64_t kMsPerSec = 1000;
    
    class NewPtsIsLarger {
    public:
        explicit NewPtsIsLarger(const I420VideoFrame* new_frame)
        : new_frame_(new_frame) {
        }
        bool operator()(I420VideoFrame* frame) {
            return (new_frame_->pts >= frame->pts);
        }
        
    private:
        const I420VideoFrame* new_frame_;
    };
    
    const uint8_t g_kuiLeadingZeroTable[256] = {
        8,  7,  6,  6,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  4,
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
        2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
    };
    
    typedef struct TagBitStringAux {
        uint8_t* pStartBuf;   // buffer to start position
        uint8_t* pEndBuf;     // buffer + length
        int32_t  iBits;       // count bits of overall bitstreaming input
        
        int32_t iIndex;      //only for cavlc usage
        uint8_t* pCurBuf;     // current reading position
        uint32_t uiCurBits;
        int32_t  iLeftBits;   // count number of available bits left ([1, 8]),
        // need pointer to next byte start position in case 0 bit left then 8 instead
    } SBitStringAux, *PBitStringAux;
    
#define GET_WORD(iCurBits, pBufPtr, iLeftBits, iAllowedBytes, iReadBytes) { \
if (iReadBytes > iAllowedBytes+1) { \
return -1; \
} \
iCurBits |= ((uint32_t)((pBufPtr[0] << 8) | pBufPtr[1])) << (iLeftBits); \
iLeftBits -= 16; \
pBufPtr +=2; \
}
    
#define WELS_READ_VERIFY(uiRet) do{ \
uint32_t uiRetTmp = (uint32_t)uiRet; \
if( uiRetTmp != 0 ) \
return uiRetTmp; \
}while(0)
    
#define NEED_BITS(iCurBits, pBufPtr, iLeftBits, iAllowedBytes, iReadBytes) { \
if (iLeftBits > 0) { \
GET_WORD(iCurBits, pBufPtr, iLeftBits, iAllowedBytes, iReadBytes); \
} \
}
#define UBITS(iCurBits, iNumBits) (iCurBits>>(32-(iNumBits)))
#define DUMP_BITS(iCurBits, pBufPtr, iLeftBits, iNumBits, iAllowedBytes, iReadBytes) { \
iCurBits <<= (iNumBits); \
iLeftBits += (iNumBits); \
NEED_BITS(iCurBits, pBufPtr, iLeftBits, iAllowedBytes, iReadBytes); \
}
    
    static inline int32_t BsGetBits (PBitStringAux pBs, int32_t iNumBits, uint32_t* pCode) {
        int32_t iRc = UBITS (pBs->uiCurBits, iNumBits);
        int32_t iAllowedBytes = pBs->pEndBuf - pBs->pStartBuf; //actual stream bytes
        int32_t iReadBytes = pBs->pCurBuf - pBs->pStartBuf;
        DUMP_BITS (pBs->uiCurBits, pBs->pCurBuf, pBs->iLeftBits, iNumBits, iAllowedBytes, iReadBytes);
        *pCode = (uint32_t)iRc;
        return 0;
    }
    
    static inline int32_t GetLeadingZeroBits (uint32_t iCurBits) { //<=32 bits
        uint32_t  uiValue;
        
        uiValue = UBITS (iCurBits, 8); //ShowBits( bs, 8 );
        if (uiValue) {
            return g_kuiLeadingZeroTable[uiValue];
        }
        
        uiValue = UBITS (iCurBits, 16); //ShowBits( bs, 16 );
        if (uiValue) {
            return (g_kuiLeadingZeroTable[uiValue] + 8);
        }
        
        uiValue = UBITS (iCurBits, 24); //ShowBits( bs, 24 );
        if (uiValue) {
            return (g_kuiLeadingZeroTable[uiValue] + 16);
        }
        
        uiValue = iCurBits; //ShowBits( bs, 32 );
        if (uiValue) {
            return (g_kuiLeadingZeroTable[uiValue] + 24);
        }
        //ASSERT(false);  // should not go here
        return -1;
    }
    
    static inline uint32_t BsGetUe (PBitStringAux pBs, uint32_t* pCode) {
        uint32_t iValue = 0;
        int32_t  iLeadingZeroBits = GetLeadingZeroBits (pBs->uiCurBits);
        int32_t iAllowedBytes, iReadBytes;
        iAllowedBytes = pBs->pEndBuf - pBs->pStartBuf; //actual stream bytes
        
        if (iLeadingZeroBits == -1) { //bistream error
            return -1;//-1
        } else if (iLeadingZeroBits >
                   16) { //rarely into this condition (even may be bitstream error), prevent from 16-bit reading overflow
            //using two-step reading instead of one time reading of >16 bits.
            iReadBytes = pBs->pCurBuf - pBs->pStartBuf;
            DUMP_BITS (pBs->uiCurBits, pBs->pCurBuf, pBs->iLeftBits, 16, iAllowedBytes, iReadBytes);
            iReadBytes = pBs->pCurBuf - pBs->pStartBuf;
            DUMP_BITS (pBs->uiCurBits, pBs->pCurBuf, pBs->iLeftBits, iLeadingZeroBits + 1 - 16, iAllowedBytes, iReadBytes);
        } else {
            iReadBytes = pBs->pCurBuf - pBs->pStartBuf;
            DUMP_BITS (pBs->uiCurBits, pBs->pCurBuf, pBs->iLeftBits, iLeadingZeroBits + 1, iAllowedBytes, iReadBytes);
        }
        if (iLeadingZeroBits) {
            iValue = UBITS (pBs->uiCurBits, iLeadingZeroBits);
            iReadBytes = pBs->pCurBuf - pBs->pStartBuf;
            DUMP_BITS (pBs->uiCurBits, pBs->pCurBuf, pBs->iLeftBits, iLeadingZeroBits, iAllowedBytes, iReadBytes);
        }
        
        *pCode = ((1u << iLeadingZeroBits) - 1 + iValue);
        return 0;
    }
    
    int ParseSliceHeader (PBitStringAux pBs, int& sliceType, int &poc) {
        int uiLog2MaxFrameNum = 4; //how many bits for framenum
        int iLog2MaxPocLsb = 6; //how many bits for poc lsb
        
        // first_mb_in_slice
        uint32_t uiCode;
        WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //first_mb_in_slice
        //  printf("yinyinyin first mb = %d\n", uiCode);
//        printf("yinyinyin first mb = %d\n", uiCode);
        
        
        // slice type
        WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //slice_type
//        printf("yinyinyin slice type = %d\n", uiCode);
        sliceType = uiCode;
        
        // pps id
        WELS_READ_VERIFY (BsGetUe (pBs, &uiCode));
//        printf("yinyinyin pps id = %d\n", uiCode);
        
        // frame number
        WELS_READ_VERIFY (BsGetBits (pBs, uiLog2MaxFrameNum, &uiCode)); //frame_num
//        printf("yinyinyin frame num = %d\n", uiCode);
        
        // picture order count
        WELS_READ_VERIFY (BsGetBits (pBs, iLog2MaxPocLsb, &uiCode)); //pic_order_cnt_lsb
//        printf("yinyinyin poc= %d\n", uiCode);
        poc = uiCode;
        
        return uiCode;
    }
    
    inline uint32_t GetValue4Bytes (uint8_t* pDstNal) {
        uint32_t uiValue = 0;
        uiValue = (pDstNal[0] << 24) | (pDstNal[1] << 16) | (pDstNal[2] << 8) | (pDstNal[3]);
        return uiValue;
    }
    
    int32_t InitReadBits (PBitStringAux pBitString, int32_t iEndOffset) {
        if (pBitString->pCurBuf >= (pBitString->pEndBuf - iEndOffset)) {
            return -1;
        }
        pBitString->uiCurBits  = GetValue4Bytes (pBitString->pCurBuf);
        pBitString->pCurBuf  += 4;
        pBitString->iLeftBits = -16;
        return 0;
    }
    
    // Convenience function for creating a dictionary.
    inline CFDictionaryRef CreateCFDictionary(CFTypeRef* keys,
                                              CFTypeRef* values,
                                              size_t size) {
        return CFDictionaryCreate(nullptr, keys, values, size,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
    }
    
    enum sliceType {
        SliceTypeI = 7,
        SliceTypeP = 5,
        sliceTypeB = 6,
        sliceTypeUnknown
    };
    
    // Struct that we pass to the decoder per frame to decode. We receive it again
    // in the decoder callback.
    struct FrameDecodeParams {
        FrameDecodeParams(DecodedImageCallback* cb, int64_t ts)
        : callback(cb), timestamp(ts) {}
        
        FrameDecodeParams() {}
        
            DecodedImageCallback* callback;
            int64_t timestamp;
            int64_t ntp_time_ms;
            uint64_t pts;
            bool idr;
    };
    
    // This is the callback function that VideoToolbox calls when decode is
    // complete.
    void VTDecompressionOutputCallback(void* decoder,
                                       void* params,
                                       OSStatus status,
                                       VTDecodeInfoFlags info_flags,
                                       CVImageBufferRef image_buffer,
                                       CMTime timestamp,
                                       CMTime duration) {
//        printf("sean decoder %p\n", decoder);
        FrameDecodeParams* decode_params(
                                         reinterpret_cast<FrameDecodeParams*>(params));
        if (status != noErr) {
            LOG(LS_ERROR) << "Failed to decode frame. Status: " << status;
            return;
        }
//        printf("current pts %llu\n", decode_params->pts);
        // TODO(tkchin): Handle CVO properly.
        scoped_refptr<VideoFrameBuffer> buffer2 =
        new RefCountedObject<CoreVideoFrameBuffer>(image_buffer);
        scoped_refptr<VideoFrameBuffer> buffer = buffer2->NativeToI420Buffer();
        I420VideoFrame *decoded_frame = new I420VideoFrame;
        
        int size_y = buffer->height()*buffer->StrideY();
        int size_u = ((buffer->height()+1)/2)*buffer->StrideU();
        int size_v = ((buffer->height()+1)/2)*buffer->StrideV();
        decoded_frame->CreateFrame(size_y, buffer->DataY(), size_u, buffer->DataU(), size_v, buffer->DataV(), buffer->width(), buffer->height(), buffer->StrideY(), buffer->StrideU(), buffer->StrideV());
        decoded_frame->set_timestamp(decode_params->timestamp);
        
        decoded_frame->set_ntp_time_ms( CMTimeGetSeconds(timestamp) * kMsPerSec);
		decoded_frame->set_ntp_time_ms(decode_params->ntp_time_ms);
        decoded_frame->pts = decode_params->pts;
        
        decode_params->callback->Decoded(*decoded_frame);
        delete decoded_frame;
        delete decode_params;
    }
    
}  // namespace internal

namespace cloopenwebrtc {
    H264VideoToolboxDecoder* H264VideoToolboxDecoder::Create() {
        return new H264VideoToolboxDecoder();
    }
    
    
    H264VideoToolboxDecoder::H264VideoToolboxDecoder()
    : callback_(nullptr),
    video_format_(nullptr),
    decompression_session_(nullptr),
    prevPicOrderCntLsb(0),
    prevPicOrderCntMsb(64),
#if DEBUG_H264
    debug_h264_(NULL),
#endif
    threshhold(8){
#if DEBUG_H264
        if (g_h264file) {
            debug_h264_ = fopen(g_h264file, "wb");
        }
#endif
    
    }
    
    H264VideoToolboxDecoder::~H264VideoToolboxDecoder() {
        DestroyDecompressionSession();
        SetVideoFormat(nullptr);
#if DEBUG_H264
        if (debug_h264_) {
            fflush(debug_h264_);
            fclose(debug_h264_);
        }
#endif
        std::list<I420VideoFrame *>::iterator it = decodedList.begin();
        I420VideoFrame *temp;
        for (; it != decodedList.end(); ) {
            temp = decodedList.front();
            delete temp;
            decodedList.pop_front();
            it = decodedList.begin();
        }
    }
    
    int H264VideoToolboxDecoder::InitDecode(const VideoCodec* video_codec,
                                            int number_of_cores) {
        
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::Decode(
                                        const EncodedImage& input_image,
                                        bool missing_frames,
                                        const RTPFragmentationHeader* fragmentation,
                                        const CodecSpecificInfo* codec_specific_info,
                                        int64_t render_time_ms) {
        
        
        
        DCHECK(input_image._buffer);
#if defined(WEBRTC_IOS)
        if (!RTCIsUIApplicationActive()) {
            // Ignore all decode requests when app isn't active. In this state, the
            // hardware decoder has been invalidated by the OS.
            // Reset video format so that we won't process frames until the next
            // keyframe.
            
            /*** when return to front, if video not resend sps and pps, decoder cannot get video_format_, so, we can't set video format null. ***/
            /*** added by zhaoyou ***/
            // SetVideoFormat(nullptr);
            return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
        }
#endif
        CMVideoFormatDescriptionRef input_format = nullptr;
        if (H264AnnexBBufferHasVideoFormatDescription(input_image._buffer,
                                                      input_image._length)) {
            input_format = CreateVideoFormatDescription(input_image._buffer,
                                                        input_image._length);
            if (input_format) {
                // Check if the video format has changed, and reinitialize decoder if
                // needed.
                if (!CMFormatDescriptionEqual(input_format, video_format_)) {
                    SetVideoFormat(input_format);
                    ResetDecompressionSession();
                }
                CFRelease(input_format);
            }
        }
        if (!video_format_) {
            // We received a frame but we don't have format information so we can't
            // decode it.
            // This can happen after backgrounding. We need to wait for the next
            // sps/pps before we can resume so we request a keyframe by returning an
            // error.
            LOG(LS_WARNING) << "Missing video format. Frame with sps/pps required.";
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        CMSampleBufferRef sample_buffer = nullptr;
        if (!H264AnnexBBufferToCMSampleBuffer(input_image._buffer,
                                              input_image._length, video_format_,
                                              &sample_buffer)) {
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        DCHECK(sample_buffer);
        VTDecodeFrameFlags decode_flags =
        kVTDecodeFrame_EnableAsynchronousDecompression;
        FrameDecodeParams* frame_decode_params;
        frame_decode_params = new FrameDecodeParams(callback_, input_image._timeStamp);
        
        //do decode image.
        OSStatus status = VTDecompressionSessionDecodeFrame(
                                                            decompression_session_, sample_buffer, decode_flags,
                                                            frame_decode_params, nullptr);
#if defined(WEBRTC_IOS)
        // Re-initialize the decoder if we have an invalid session while the app is
        // active and retry the decode request.
        if (status == kVTInvalidSessionErr &&
            ResetDecompressionSession() == WEBRTC_VIDEO_CODEC_OK) {
            frame_decode_params = new FrameDecodeParams(callback_, input_image._timeStamp);
            status = VTDecompressionSessionDecodeFrame(
                                                       decompression_session_, sample_buffer, decode_flags,
                                                       frame_decode_params, nullptr);
        }
#endif
        CFRelease(sample_buffer);
        if (status != noErr) {
            LOG(LS_ERROR) << "Failed to decode frame with code: " << status;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::RegisterDecodeCompleteCallback(
                                                                DecodedImageCallback* callback) {
        DCHECK(!callback_);
        callback_ = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::Release() {
        // Need to invalidate the session so that callbacks no longer occur and it
        // is safe to null out the callback.
        DestroyDecompressionSession();
        SetVideoFormat(nullptr);
        callback_ = nullptr;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::ResetDecompressionSession() {
        DestroyDecompressionSession();
        
        // Need to wait for the first SPS to initialize decoder.
        if (!video_format_) {
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        // Set keys for OpenGL and IOSurface compatibilty, which makes the encoder
        // create pixel buffers with GPU backed memory. The intent here is to pass
        // the pixel buffers directly so we avoid a texture upload later during
        // rendering. This currently is moot because we are converting back to an
        // I420 frame after decode, but eventually we will be able to plumb
        // CVPixelBuffers directly to the renderer.
        // TODO(tkchin): Maybe only set OpenGL/IOSurface keys if we know that that
        // we can pass CVPixelBuffers as native handles in decoder output.
        static size_t const attributes_size = 3;
        CFTypeRef keys[attributes_size] = {
#if defined(WEBRTC_IOS)
            kCVPixelBufferOpenGLESCompatibilityKey,
#elif defined(WEBRTC_MAC)
            kCVPixelBufferOpenGLCompatibilityKey,
#endif
            kCVPixelBufferIOSurfacePropertiesKey,
            kCVPixelBufferPixelFormatTypeKey
        };
        CFDictionaryRef io_surface_value =
        CreateCFDictionary(nullptr, nullptr, 0);
        int64_t nv12type = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        CFNumberRef pixel_format = CFNumberCreate(nullptr, kCFNumberLongType, &nv12type);
        CFTypeRef values[attributes_size] = {kCFBooleanTrue, io_surface_value,
            pixel_format};
        CFDictionaryRef attributes = CreateCFDictionary(keys, values, attributes_size);
        if (io_surface_value) {
            CFRelease(io_surface_value);
            io_surface_value = nullptr;
        }
        if (pixel_format) {
            CFRelease(pixel_format);
            pixel_format = nullptr;
        }
        VTDecompressionOutputCallbackRecord record = {
            VTDecompressionOutputCallback, this,
        };
        OSStatus status =
        VTDecompressionSessionCreate(nullptr, video_format_, nullptr, attributes,
                                     &record, &decompression_session_);
        CFRelease(attributes);
        if (status != noErr) {
            DestroyDecompressionSession();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        ConfigureDecompressionSession();
        
        
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    void H264VideoToolboxDecoder::ConfigureDecompressionSession() {
        DCHECK(decompression_session_);
#if defined(WEBRTC_IOS)
        VTSessionSetProperty(decompression_session_,
                             kVTDecompressionPropertyKey_RealTime, kCFBooleanTrue);
#endif
    }
    
    void H264VideoToolboxDecoder::DestroyDecompressionSession() {
        if (decompression_session_) {
            VTDecompressionSessionWaitForAsynchronousFrames(decompression_session_);
            CFRelease(decompression_session_);
            decompression_session_ = nullptr;
        }
    }
    
    void H264VideoToolboxDecoder::SetVideoFormat(
                                                 CMVideoFormatDescriptionRef video_format) {
        if (video_format_ == video_format) {
            return;
        }
        if (video_format_) {
            CFRelease(video_format_);
        }
        video_format_ = video_format;
        if (video_format_) {
            CFRetain(video_format_);
        }
    }
    
    const char* H264VideoToolboxDecoder::ImplementationName() const {
        return "VideoToolbox";
    }
    
    WebRtc_Word32
    H264VideoToolboxDecoder::Reset()
    {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
}  // namespace webrtc

#endif  // defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
