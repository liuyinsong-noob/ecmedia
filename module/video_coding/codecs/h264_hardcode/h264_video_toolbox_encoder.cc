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

#include "h264_video_toolbox_encoder.h"

#if defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)

#include <memory>
#include <string>
#include <vector>

#if defined(WEBRTC_IOS)
#include "RTCUIApplication.h"
#endif
#include "convert_from.h"
#include "checks.h"
#include "logging.h"
#include "h264_video_toolbox_nalu.h"
#include "clock.h"
#include "libyuv.h"

namespace cloopenwebrtc
{extern int printTime();}

namespace internal {
    
// Convenience function for creating a dictionary.
inline CFDictionaryRef CreateCFDictionary(CFTypeRef* keys,
                                          CFTypeRef* values,
                                          size_t size) {
  return CFDictionaryCreate(kCFAllocatorDefault, keys, values, size,
                            &kCFTypeDictionaryKeyCallBacks,
                            &kCFTypeDictionaryValueCallBacks);
}

// Copies characters from a CFStringRef into a std::string.
std::string CFStringToString(const CFStringRef cf_string) {
  DCHECK(cf_string);
  std::string std_string;
  // Get the size needed for UTF8 plus terminating character.
  size_t buffer_size =
      CFStringGetMaximumSizeForEncoding(CFStringGetLength(cf_string),
                                        kCFStringEncodingUTF8) +
      1;
  char* buffer(new char[buffer_size]);
  if (CFStringGetCString(cf_string, buffer, buffer_size,
                         kCFStringEncodingUTF8)) {
    // Copy over the characters.
    std_string.assign(buffer);
  }
  return std_string;
}

// Convenience function for setting a VT property.
void SetVTSessionProperty(VTSessionRef session,
                          CFStringRef key,
                          int32_t value) {
  CFNumberRef cfNum =
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
  OSStatus status = VTSessionSetProperty(session, key, cfNum);
  CFRelease(cfNum);
  if (status != noErr) {
    std::string key_string = CFStringToString(key);
    LOG(LS_ERROR) << "VTSessionSetProperty failed to set: " << key_string
                  << " to " << value << ": " << status;
      
      printf("VTSessionSetProperty failed to set: %s to %d, error %d\n", key_string.c_str(), value, status);
  }
}

// Convenience function for setting a VT property.
void SetVTSessionProperty(VTSessionRef session,
                          CFStringRef key,
                          uint32_t value) {
  int64_t value_64 = value;
  CFNumberRef cfNum =
      CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &value_64);
  OSStatus status = VTSessionSetProperty(session, key, cfNum);
  CFRelease(cfNum);
  if (status != noErr) {
    std::string key_string = CFStringToString(key);
    LOG(LS_ERROR) << "VTSessionSetProperty failed to set: " << key_string
                  << " to " << value << ": " << status;
      printf("VTSessionSetProperty failed to set: %s to %d, error %d\n", key_string.c_str(), value, status);
  }
}

// Convenience function for setting a VT property.
void SetVTSessionProperty(VTSessionRef session, CFStringRef key, bool value) {
  CFBooleanRef cf_bool = (value) ? kCFBooleanTrue : kCFBooleanFalse;
  OSStatus status = VTSessionSetProperty(session, key, cf_bool);
  if (status != noErr) {
    std::string key_string = CFStringToString(key);
    LOG(LS_ERROR) << "VTSessionSetProperty failed to set: " << key_string
                  << " to " << value << ": " << status;
      printf("VTSessionSetProperty failed to set: %s to %d, error %d\n", key_string.c_str(), value, status);
  }
}

// Convenience function for setting a VT property.
void SetVTSessionProperty(VTSessionRef session,
                          CFStringRef key,
                          CFStringRef value) {
  OSStatus status = VTSessionSetProperty(session, key, value);
  if (status != noErr) {
    std::string key_string = CFStringToString(key);
    std::string val_string = CFStringToString(value);
    LOG(LS_ERROR) << "VTSessionSetProperty failed to set: " << key_string
                  << " to " << val_string << ": " << status;
      printf("VTSessionSetProperty failed to set: %s to %d, error %d\n", key_string.c_str(), value, status);
  }
}

// Struct that we pass to the encoder per frame to encode. We receive it again
// in the encoder callback.
struct FrameEncodeParams {
    FrameEncodeParams(cloopenwebrtc::H264VideoToolboxEncoder* e,
                      const cloopenwebrtc::CodecSpecificInfo* csi,
                    int32_t w,
                    int32_t h,
                    int64_t rtms,
                    uint32_t ts/*,
                    cloopenwebrtc::VideoRotation r*/)
      : encoder(e),
        width(w),
        height(h),
        render_time_ms(rtms),
        timestamp(ts)/*,
        rotation(r)*/ {
    if (csi) {
      codec_specific_info = *csi;
    } else {
      codec_specific_info.codecType = cloopenwebrtc::kVideoCodecH264;
    }
  }

  cloopenwebrtc::H264VideoToolboxEncoder* encoder;
  cloopenwebrtc::CodecSpecificInfo codec_specific_info;
  int32_t width;
  int32_t height;
  int64_t render_time_ms;
  uint32_t timestamp;
//  cloopenwebrtc::VideoRotation rotation;
};

// We receive I420Frames as input, but we need to feed CVPixelBuffers into the
// encoder. This performs the copy and format conversion.
// TODO(tkchin): See if encoder will accept i420 frames and compare performance.
bool CopyVideoFrameToPixelBuffer(const cloopenwebrtc::I420VideoFrame& frame,
                                 CVPixelBufferRef pixel_buffer) {
  DCHECK(pixel_buffer);
  DCHECK(CVPixelBufferGetPixelFormatType(pixel_buffer) ==
             kCVPixelFormatType_420YpCbCr8BiPlanarFullRange);
  DCHECK(CVPixelBufferGetHeightOfPlane(pixel_buffer, 0) ==
             static_cast<size_t>(frame.height()));
  DCHECK(CVPixelBufferGetWidthOfPlane(pixel_buffer, 0) ==
             static_cast<size_t>(frame.width()));

  CVReturn cvRet = CVPixelBufferLockBaseAddress(pixel_buffer, 0);
  if (cvRet != kCVReturnSuccess) {
    LOG(LS_ERROR) << "Failed to lock base address: " << cvRet;
    return false;
  }
  uint8_t* dst_y = reinterpret_cast<uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
  size_t dst_stride_y = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0);
  uint8_t* dst_uv = reinterpret_cast<uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
  size_t dst_stride_uv = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1);
  // Convert I420 to NV12.
    int ret = libyuv::I420ToNV12(
      frame.buffer(cloopenwebrtc::kYPlane), frame.stride(cloopenwebrtc::kYPlane),
      frame.buffer(cloopenwebrtc::kUPlane), frame.stride(cloopenwebrtc::kUPlane),
      frame.buffer(cloopenwebrtc::kVPlane), frame.stride(cloopenwebrtc::kVPlane), dst_y,
      (int)dst_stride_y, dst_uv, (int)dst_stride_uv, frame.width(), frame.height());
  CVPixelBufferUnlockBaseAddress(pixel_buffer, 0);
  if (ret) {
    LOG(LS_ERROR) << "Error converting I420 VideoFrame to NV12 :" << ret;
    return false;
  }
  return true;
}

// This is the callback function that VideoToolbox calls when encode is
// complete. From inspection this happens on its own queue.
void VTCompressionOutputCallback(void* encoder,
                                 void* params,
                                 OSStatus status,
                                 VTEncodeInfoFlags info_flags,
                                 CMSampleBufferRef sample_buffer) {
    CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sample_buffer);
    size_t frame_size = CMBlockBufferGetDataLength(block);
    static int count = 0;
    static int total = 0;
    total += frame_size;
    if (count++ == 15) {
        cloopenwebrtc::printTime(); printf("%d\n",total);
        count = 0;
        total = 0;
    }
    
  FrameEncodeParams * encode_params(
      reinterpret_cast<FrameEncodeParams*>(params));
  encode_params->encoder->OnEncodedFrame(
      status, info_flags, sample_buffer, encode_params->codec_specific_info,
      encode_params->width, encode_params->height,
      encode_params->render_time_ms, encode_params->timestamp/*,
      encode_params->rotation*/);
    delete encode_params;
}

}  // namespace internal

namespace cloopenwebrtc {

    H264VideoToolboxEncoder* H264VideoToolboxEncoder::Create() {
        return new H264VideoToolboxEncoder();
    }
    
// .5 is set as a mininum to prevent overcompensating for large temporary
// overshoots. We don't want to degrade video quality too badly.
// .95 is set to prevent oscillations. When a lower bitrate is set on the
// encoder than previously set, its output seems to have a brief period of
// drastically reduced bitrate, so we want to avoid that. In steady state
// conditions, 0.95 seems to give us better overall bitrate over long periods
// of time.
H264VideoToolboxEncoder::H264VideoToolboxEncoder()
    : callback_(nullptr),
      compression_session_(nullptr),
      bitrate_adjuster_(Clock::GetRealTimeClock(), .5, .95) {}

H264VideoToolboxEncoder::~H264VideoToolboxEncoder() {
  DestroyCompressionSession();
}

int H264VideoToolboxEncoder::InitEncode(const VideoCodec* codec_settings,
                                        int number_of_cores,
                                        size_t max_payload_size) {
  DCHECK(codec_settings);
  DCHECK_EQ(codec_settings->codecType, kVideoCodecH264);
  // TODO(tkchin): We may need to enforce width/height dimension restrictions
  // to match what the encoder supports.
  width_ = codec_settings->width;
  height_ = codec_settings->height;
  // We can only set average bitrate on the HW encoder.
  target_bitrate_bps_ = codec_settings->startBitrate;
  bitrate_adjuster_.SetTargetBitrateBps(target_bitrate_bps_);
    
  // TODO(tkchin): Try setting payload size via
  // kVTCompressionPropertyKey_MaxH264SliceBytes.

  return ResetCompressionSession();
}

WebRtc_Word32 H264VideoToolboxEncoder::Encode(
    const I420VideoFrame& input_image,
    const CodecSpecificInfo* codec_specific_info,
    const std::vector<VideoFrameType>* frame_types) {
  if (input_image.IsZeroSize()) {
    // It's possible to get zero sizes as a signal to produce keyframes (this
    // happens for internal sources). But this shouldn't happen in
    // webrtcvideoengine2.
    NOTREACHED();
    return WEBRTC_VIDEO_CODEC_OK;
  }
  if (!callback_ || !compression_session_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
#if defined(WEBRTC_IOS)
  if (!RTCIsUIApplicationActive()) {
    // Ignore all encode requests when app isn't active. In this state, the
    // hardware encoder has been invalidated by the OS.
    return WEBRTC_VIDEO_CODEC_OK;
  }
#endif
  bool is_keyframe_required = false;
  // Get a pixel buffer from the pool and copy frame data over.
  CVPixelBufferPoolRef pixel_buffer_pool =
      VTCompressionSessionGetPixelBufferPool(compression_session_);
#if defined(WEBRTC_IOS)
  if (!pixel_buffer_pool) {
    // Kind of a hack. On backgrounding, the compression session seems to get
    // invalidated, which causes this pool call to fail when the application
    // is foregrounded and frames are being sent for encoding again.
    // Resetting the session when this happens fixes the issue.
    // In addition we request a keyframe so video can recover quickly.
    ResetCompressionSession();
    pixel_buffer_pool =
        VTCompressionSessionGetPixelBufferPool(compression_session_);
    is_keyframe_required = true;
  }
#endif
  if (!pixel_buffer_pool) {
    LOG(LS_ERROR) << "Failed to get pixel buffer pool.";
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  CVPixelBufferRef pixel_buffer = nullptr;
  CVReturn ret = CVPixelBufferPoolCreatePixelBuffer(nullptr, pixel_buffer_pool,
                                                    &pixel_buffer);
  if (ret != kCVReturnSuccess) {
    LOG(LS_ERROR) << "Failed to create pixel buffer: " << ret;
    // We probably want to drop frames here, since failure probably means
    // that the pool is empty.
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  DCHECK(pixel_buffer);
  if (!::internal::CopyVideoFrameToPixelBuffer(input_image, pixel_buffer)) {
    LOG(LS_ERROR) << "Failed to copy frame data.";
    CVBufferRelease(pixel_buffer);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  // Check if we need a keyframe.
  if (!is_keyframe_required && frame_types) {
    for (auto frame_type : *frame_types) {
      if (frame_type == kKeyFrame) {
        is_keyframe_required = true;
        break;
      }
    }
  }

  CMTime presentation_time_stamp =
      CMTimeMake(input_image.render_time_ms(), 1000);
  CFDictionaryRef frame_properties = nullptr;
  if (is_keyframe_required) {
    CFTypeRef keys[] = {kVTEncodeFrameOptionKey_ForceKeyFrame};
    CFTypeRef values[] = {kCFBooleanTrue};
      frame_properties = ::internal::CreateCFDictionary(keys, values, 1);
  }
    ::internal::FrameEncodeParams* encode_params = new ::internal::FrameEncodeParams(
      this, codec_specific_info, width_, height_, input_image.render_time_ms(),
      input_image.timestamp()/*, input_image.rotation()*/);

  // Update the bitrate if needed.
  SetBitrateBps(bitrate_adjuster_.GetAdjustedBitrateBps());

  OSStatus status = VTCompressionSessionEncodeFrame(
      compression_session_, pixel_buffer, presentation_time_stamp,
      kCMTimeInvalid, frame_properties, encode_params, nullptr);
  if (frame_properties) {
    CFRelease(frame_properties);
  }
  if (pixel_buffer) {
    CVBufferRelease(pixel_buffer);
  }
//    delete encode_params;
  if (status != noErr) {
    LOG(LS_ERROR) << "Failed to encode frame with code: " << status;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int H264VideoToolboxEncoder::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int H264VideoToolboxEncoder::SetChannelParameters(uint32_t packet_loss,
                                                  int64_t rtt) {
  // Encoder doesn't know anything about packet loss or rtt so just return.
  return WEBRTC_VIDEO_CODEC_OK;
}

int H264VideoToolboxEncoder::SetRates(uint32_t new_bitrate_kbit,
                                      uint32_t frame_rate) {
    
  target_bitrate_bps_ = 1000 * new_bitrate_kbit;
    printTime();printf("new bit rate %d\n", target_bitrate_bps_);
  bitrate_adjuster_.SetTargetBitrateBps(target_bitrate_bps_);
  SetBitrateBps(bitrate_adjuster_.GetAdjustedBitrateBps());

  return WEBRTC_VIDEO_CODEC_OK;
}

int H264VideoToolboxEncoder::Release() {
  // Need to reset so that the session is invalidated and won't use the
  // callback anymore. Do not remove callback until the session is invalidated
  // since async encoder callbacks can occur until invalidation.
  int ret = ResetCompressionSession();
  callback_ = nullptr;
  return ret;
}

WebRtc_Word32 H264VideoToolboxEncoder::SetPeriodicKeyFrames(bool enable)
{
//    periodicKeyFrames_ = enable;
    return WEBRTC_VIDEO_CODEC_OK;
}

    
int H264VideoToolboxEncoder::ResetCompressionSession() {
  DestroyCompressionSession();

  // Set source image buffer attributes. These attributes will be present on
  // buffers retrieved from the encoder's pixel buffer pool.
  const size_t attributes_size = 3;
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
    ::internal::CreateCFDictionary(nullptr, nullptr, 0);
  int64_t nv12type = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
  CFNumberRef pixel_format =
      CFNumberCreate(nullptr, kCFNumberLongType, &nv12type);
  CFTypeRef values[attributes_size] = {kCFBooleanTrue, io_surface_value,
                                       pixel_format};
  CFDictionaryRef source_attributes =
    ::internal::CreateCFDictionary(keys, values, attributes_size);
  if (io_surface_value) {
    CFRelease(io_surface_value);
    io_surface_value = nullptr;
  }
  if (pixel_format) {
    CFRelease(pixel_format);
    pixel_format = nullptr;
  }
  OSStatus status = VTCompressionSessionCreate(
      nullptr,  // use default allocator
      width_, height_, kCMVideoCodecType_H264,
      nullptr,  // use default encoder
      source_attributes,
      nullptr,  // use default compressed data allocator
                                               ::internal::VTCompressionOutputCallback, this, &compression_session_);
  if (source_attributes) {
    CFRelease(source_attributes);
    source_attributes = nullptr;
  }
  if (status != noErr) {
    LOG(LS_ERROR) << "Failed to create compression session: " << status;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  ConfigureCompressionSession();
  return WEBRTC_VIDEO_CODEC_OK;
}

void H264VideoToolboxEncoder::ConfigureCompressionSession() {
  DCHECK(compression_session_);
    ::internal::SetVTSessionProperty(compression_session_,
                                 kVTCompressionPropertyKey_RealTime, true);
    ::internal::SetVTSessionProperty(compression_session_,
                                 kVTCompressionPropertyKey_ProfileLevel,
                                 kVTProfileLevel_H264_Baseline_AutoLevel);
    ::internal::SetVTSessionProperty(compression_session_,
                                 kVTCompressionPropertyKey_AllowFrameReordering,
                                 false);
    ::internal::SetVTSessionProperty(compression_session_,
                                     kVTCompressionPropertyKey_AverageBitRate,
                                     800000);
//  SetEncoderBitrateBps(target_bitrate_bps_*1000);
  // TODO(tkchin): Look at entropy mode and colorspace matrices.
  // TODO(tkchin): Investigate to see if there's any way to make this work.
  // May need it to interop with Android. Currently this call just fails.
  // On inspecting encoder output on iOS8, this value is set to 6.
  // internal::SetVTSessionProperty(compression_session_,
  //     kVTCompressionPropertyKey_MaxFrameDelayCount,
  //     1);
  // TODO(tkchin): See if enforcing keyframe frequency is beneficial in any
  // way.
  // internal::SetVTSessionProperty(
  //     compression_session_,
  //     kVTCompressionPropertyKey_MaxKeyFrameInterval, 240);
  // internal::SetVTSessionProperty(
  //     compression_session_,
  //     kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration, 240);
}

void H264VideoToolboxEncoder::DestroyCompressionSession() {
  if (compression_session_) {
    VTCompressionSessionInvalidate(compression_session_);
    CFRelease(compression_session_);
    compression_session_ = nullptr;
  }
}

const char* H264VideoToolboxEncoder::ImplementationName() const {
  return "VideoToolbox";
}

void H264VideoToolboxEncoder::SetBitrateBps(uint32_t bitrate_bps) {
    return;
    bitrate_bps *= 1000;
    printf("bitrate_bps %d\n",bitrate_bps);
  if (encoder_bitrate_bps_ != bitrate_bps) {
      printf("bitrate_bps 222 %d\n",bitrate_bps);
    SetEncoderBitrateBps(bitrate_bps);
  }
}

void H264VideoToolboxEncoder::SetEncoderBitrateBps(uint32_t bitrate_bps) {
  if (compression_session_) {
      ::internal::SetVTSessionProperty(compression_session_,
                                   kVTCompressionPropertyKey_AverageBitRate,
                                   bitrate_bps);
    encoder_bitrate_bps_ = bitrate_bps;
  }
}

void H264VideoToolboxEncoder::OnEncodedFrame(
    OSStatus status,
    VTEncodeInfoFlags info_flags,
    CMSampleBufferRef sample_buffer,
    CodecSpecificInfo codec_specific_info,
    int32_t width,
    int32_t height,
    int64_t render_time_ms,
    uint32_t timestamp/*,
    VideoRotation rotation*/) {
//        printf("sean status %d\n",status);
  if (status != noErr) {
    LOG(LS_ERROR) << "H264 encode failed.";
    return;
  }
  if (info_flags & kVTEncodeInfo_FrameDropped) {
    LOG(LS_INFO) << "H264 encode dropped frame.";
  }

  bool is_keyframe = false;
  CFArrayRef attachments =
      CMSampleBufferGetSampleAttachmentsArray(sample_buffer, 0);
  if (attachments != nullptr && CFArrayGetCount(attachments)) {
    CFDictionaryRef attachment =
        static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
    is_keyframe =
        !CFDictionaryContainsKey(attachment, kCMSampleAttachmentKey_NotSync);
  }

  // Convert the sample buffer into a buffer suitable for RTP packetization.
  // TODO(tkchin): Allocate buffers through a pool.
  rtc::Buffer* buffer(new rtc::Buffer());
   RTPFragmentationHeader *header = new RTPFragmentationHeader();
  {
    RTPFragmentationHeader* header_raw;
    bool result = H264CMSampleBufferToAnnexBBuffer(sample_buffer, is_keyframe,
                                                   buffer, &header_raw);
    header->CopyFrom(*header_raw);
      delete header_raw;
    if (!result) {
      return;
    }
  }
  EncodedImage frame(buffer->data(), buffer->size(), buffer->size());
  frame._encodedWidth = width;
  frame._encodedHeight = height;
  frame._completeFrame = true;
        frame._frameType = is_keyframe ? kKeyFrame : kDeltaFrame;
  frame.capture_time_ms_ = render_time_ms;
  frame._timeStamp = timestamp;
//  frame.rotation_ = rotation;

  int result = callback_->Encoded(frame, &codec_specific_info, header);
        delete header;
        delete buffer;
  if (result != 0) {
    LOG(LS_ERROR) << "Encode callback failed: " << result;
    return;
  }
  bitrate_adjuster_.Update(frame._size);
}

}  // namespace webrtc

#endif  // defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
