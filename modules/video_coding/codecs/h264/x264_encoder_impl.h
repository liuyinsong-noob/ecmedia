#ifndef MODULES_VIDEO_CODING_CODECS_H264_X264_ENCODER_IMPL_H_
#define MODULES_VIDEO_CODING_CODECS_H264_X264_ENCODER_IMPL_H_

#include "api/video/i420_buffer.h"
#include "api/video_codecs/video_encoder.h"
#include "common_video/h264/h264_bitstream_parser.h"
#include "modules/video_coding/codecs/h264/include/h264.h"

struct x264_t;
struct x264_picture_t;
struct x264_param_t;
//#define  SAVE_ENCODEDE_FILE

namespace webrtc {

class X264EncoderImpl : public H264Encoder {
 public:
 public:
  struct LayerConfig {
    int simulcast_idx = 0;
    int width = -1;
    int height = -1;
    bool sending = true;
    bool key_frame_request = false;
    float max_frame_rate = 0;
    uint32_t target_bps = 0;
    uint32_t max_bps = 0;
    bool frame_dropping_on = false;
    int key_frame_interval = 0;

    void SetStreamState(bool send_stream);
  };
#ifdef SAVE_ENCODEDE_FILE
  static int encoder_seq_;
#endif
  explicit X264EncoderImpl(const cricket::VideoCodec& codec);
  ~X264EncoderImpl() override;

  // |max_payload_size| is ignored.
  // The following members of |codec_settings| are used. The rest are ignored.
  // - codecType (must be kVideoCodecH264)
  // - targetBitrate
  // - maxFramerate
  // - width
  // - height
  int32_t InitEncode(const VideoCodec* codec_settings,
                     int32_t number_of_cores,
                     size_t max_payload_size) override;
  int32_t Release() override;

  int32_t RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback) override;
  int32_t SetRateAllocation(const VideoBitrateAllocation& bitrate_allocation,
                            uint32_t framerate) override;

  // The result of encoding - an EncodedImage and RTPFragmentationHeader - are
  // passed to the encode complete callback.
  int32_t Encode(const VideoFrame& frame,
                 const std::vector<VideoFrameType>* frame_types) override;

  EncoderInfo GetEncoderInfo() const override;

 private:
  static void X264Log(void* handle, int i_level, const char* fmt, va_list vars);
  x264_param_t CreateEncoderParams(size_t i) const;
  webrtc::H264BitstreamParser h264_bitstream_parser_;

  // Reports statistics with histograms.
  void ReportInit();
  void ReportError();

  std::vector<x264_t*> encoders_;
  std::vector<x264_picture_t> pictures_;
  std::vector<x264_param_t> x264_params_;
  std::vector<rtc::scoped_refptr<I420Buffer>> downscaled_buffers_;
  std::vector<LayerConfig> configurations_;
  std::vector<EncodedImage> encoded_images_;

  VideoCodec codec_;
  H264PacketizationMode packetization_mode_;
  size_t max_payload_size_;
  int32_t number_of_cores_;
  EncodedImageCallback* encoded_image_callback_;

  bool has_reported_init_;
  bool has_reported_error_;

  int num_temporal_layers_;
  uint8_t tl0sync_limit_;

  int count_;
};

}  // namespace webrtc

#endif