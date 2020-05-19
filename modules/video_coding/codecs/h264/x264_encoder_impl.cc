#include "modules/video_coding/codecs/h264/x264_encoder_impl.h"

#include <cstdio>
#include <cstdlib>
#include "absl/strings/match.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"
#include "modules/video_coding/utility/simulcast_utility.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv/scale.h"

#include "rtc_base/time_utils.h"
// before using x264-svc, must define macro RLCLOUD

//#define RLCLOUD 1
#define _RLCLOUD 1
#ifdef _RLCLOUD
//#ifdef RLCLOUD
#include "third_party/libx264svc_win/x86/include/x264.h"
#else
#include "third_party/libx264_win/x86/include/x264.h"
#endif

namespace webrtc {
#ifdef SAVE_ENCODEDE_FILE
	int X264EncoderImpl::encoder_seq_ = 0;
#endif
namespace {
// QP scaling thresholds.
static const int kLowH264QpThreshold = 24;
static const int kHighH264QpThreshold = 37;

VideoFrameType ConvertToVideoFrameType(int i_type) {
  switch (i_type) {
    case X264_TYPE_IDR:
      return VideoFrameType::kVideoFrameKey;
    case X264_TYPE_KEYFRAME:  // Fix: is b_open_gop
    case X264_TYPE_I:
    case X264_TYPE_P:
    case X264_TYPE_B:
    case X264_TYPE_BREF:
      return VideoFrameType::kVideoFrameDelta;
    default:
      break;
  }
  RTC_NOTREACHED() << "Unexpected/invalid frame type: " << i_type;
  return VideoFrameType::kEmptyFrame;
}
}  // namespace

// Helper method used by H264EncoderImpl::Encode.
// Copies the encoded bytes from |info| to |encoded_image| and updates the
// fragmentation information of |frag_header|. The |encoded_image->_buffer|
// may be deleted and reallocated if a bigger buffer is required.
//
// After OpenH264 encoding, the encoded bytes are stored in |info| spread
// out over a number of layers and "NAL units". Each NAL unit is a fragment
// starting with the four-byte start code {0,0,0,1}. All of this data
// (including the start codes) is copied to the |encoded_image->_buffer| and
// the |frag_header| is updated to point to each fragment, with offsets and
// lengths set as to exclude the start codes.
static void RtpFragmentize(EncodedImage* encoded_image,
                           const VideoFrameBuffer& frame_buffer,
                           x264_nal_t* nals_out,
                           int num_nals,
                           RTPFragmentationHeader* frag_header) {
  // Calculate minimum buffer size required to hold encoded data.
  size_t required_capacity = 0;
  size_t fragments_count = 0;

  for (int i = 0; i < num_nals; i++) {
    x264_nal_t* current_nal = &nals_out[i];
    RTC_CHECK_GE(current_nal->i_payload, 0);
    // Ensure |required_capacity| will not overflow.
    RTC_CHECK_LE(current_nal->i_payload,
                 std::numeric_limits<size_t>::max() - required_capacity);
    required_capacity += current_nal->i_payload;
  }

  if (encoded_image->capacity() < required_capacity) {
    // Increase buffer size. Allocate enough to hold an unencoded image, this
    // should be more than enough to hold any encoded data of future frames of
    // the same size (avoiding possible future reallocation due to variations in
    // required size).
    size_t new_capacity = CalcBufferSize(VideoType::kI420, frame_buffer.width(),
                                         frame_buffer.height());
    if (new_capacity < required_capacity) {
      // Encoded data > unencoded data. Allocate required bytes.
      RTC_LOG(LS_WARNING)
          << "X264 Encoding produced more bytes than the original image "
          << "data! Original bytes: " << new_capacity
          << ", encoded bytes: " << required_capacity << ".";
      new_capacity = required_capacity;
    }
    encoded_image->Allocate(new_capacity);
  }

  // Iterate layers and NAL units, note each NAL unit as a fragment and copy
  // the data to |encoded_image->_buffer|.
  const uint8_t start_code[4] = {0, 0, 0, 1};
  frag_header->VerifyAndAllocateFragmentationHeader(fragments_count);
  size_t frag = 0;
  encoded_image->set_size(0);

  // Iterate NAL units making up this frame
  for (int i = 0; i < num_nals; ++i, ++frag) {
    x264_nal_t* current_nal = &nals_out[i];
    // Because the sum of all layer lengths, |required_capacity|, fits in a
    // |size_t|, we know that any indices in-between will not overflow.
    int nal_size = current_nal->i_payload;
    rtc::CopyOnWriteBuffer nal_buffer;

    if (!current_nal->b_long_startcode)
      nal_size++;
    nal_buffer.SetSize(nal_size);

    if (current_nal->b_long_startcode)
      memcpy(nal_buffer.data(), current_nal->p_payload, current_nal->i_payload);
    else {
      memcpy(nal_buffer.data(), start_code, 4);
      memcpy(nal_buffer.data() + 4, current_nal->p_payload + 3,
             current_nal->i_payload - 3);
    }

    RTC_DCHECK_GE(nal_size, 4);
    RTC_DCHECK_EQ(nal_buffer[0], start_code[0]);
    RTC_DCHECK_EQ(nal_buffer[1], start_code[1]);
    RTC_DCHECK_EQ(nal_buffer[2], start_code[2]);
    RTC_DCHECK_EQ(nal_buffer[3], start_code[3]);
    frag_header->fragmentationOffset[frag] =
        encoded_image->size() + sizeof(start_code);
    frag_header->fragmentationLength[frag] = nal_size - sizeof(start_code);

    // Copy the nal's data (including start codes).
    memcpy(encoded_image->data() + encoded_image->size(), nal_buffer.data(),
           nal_size);
    encoded_image->set_size(encoded_image->size() + nal_size);
  }
}

X264EncoderImpl::X264EncoderImpl(const cricket::VideoCodec& codec)
    : packetization_mode_(H264PacketizationMode::SingleNalUnit),
      max_payload_size_(0),
      number_of_cores_(0),
      encoded_image_callback_(nullptr),
      has_reported_init_(false),
      has_reported_error_(false),
      num_temporal_layers_(1),
      tl0sync_limit_(0),
		count_(0) {
#ifdef SAVE_ENCODEDE_FILE
	encoder_seq_++;
#endif
  RTC_CHECK(absl::EqualsIgnoreCase(codec.name, cricket::kH264CodecName));
  std::string packetization_mode_string;
  if (codec.GetParam(cricket::kH264FmtpPacketizationMode,
                     &packetization_mode_string) &&
      packetization_mode_string == "1") {
    packetization_mode_ = H264PacketizationMode::NonInterleaved;
  }
  packetization_mode_ = H264PacketizationMode::NonInterleaved;
  downscaled_buffers_.reserve(kMaxSimulcastStreams - 1);
  encoded_images_.reserve(kMaxSimulcastStreams);
  encoders_.reserve(kMaxSimulcastStreams);
  configurations_.reserve(kMaxSimulcastStreams);
}

X264EncoderImpl::~X264EncoderImpl() {
  Release();
}

int32_t X264EncoderImpl::InitEncode(const VideoCodec* inst,
                                    int32_t number_of_cores,
                                    size_t max_payload_size) {
  ReportInit();
  if (!inst || inst->codecType != kVideoCodecH264) {
    ReportError();
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->maxFramerate == 0) {
    ReportError();
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->width < 1 || inst->height < 1) {
    ReportError();
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  int32_t release_ret = Release();
  if (release_ret != WEBRTC_VIDEO_CODEC_OK) {
    ReportError();
    return release_ret;
  }

  int number_of_streams = SimulcastUtility::NumberOfSimulcastStreams(*inst);
  bool doing_simulcast = (number_of_streams > 1);

  if (doing_simulcast &&
      !SimulcastUtility::ValidSimulcastParameters(*inst, number_of_streams)) {
    return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
  }
  downscaled_buffers_.resize(number_of_streams - 1);
  encoded_images_.resize(number_of_streams);
  encoders_.resize(number_of_streams);
  pictures_.resize(number_of_streams);
  configurations_.resize(number_of_streams);

  number_of_cores_ = number_of_cores;
  max_payload_size_ = max_payload_size;
  codec_ = *inst;

  // Code expects simulcastStream resolutions to be correct, make sure they are
  // filled even when there are no simulcast layers.
  if (codec_.numberOfSimulcastStreams == 0) {
    codec_.simulcastStream[0].width = codec_.width;
    codec_.simulcastStream[0].height = codec_.height;
  }

  num_temporal_layers_ = codec_.H264()->numberOfTemporalLayers;

#ifdef SAVE_ENCODEDE_FILE
  for (int tid = 0; tid < num_temporal_layers_; tid++) {
    std::string file_name("x264svc_t");
    file_name += std::to_string(tid) + std::string(".264");
    std::ofstream ofile(file_name, std::ios::binary | std::ios::out);
    if (!ofile) {
      RTC_LOG(LS_INFO) << "failed to open file: " << file_name;
    } else
      output_temporal_files_.push_back(std::move(ofile));
  }

#endif

  for (int i = 0, idx = number_of_streams - 1; i < number_of_streams;
       ++i, --idx) {
    // Set internal settings from codec_settings
    configurations_[i].simulcast_idx = idx;
    configurations_[i].sending = false;
    configurations_[i].width = codec_.simulcastStream[idx].width;
    configurations_[i].height = codec_.simulcastStream[idx].height;
    configurations_[i].max_frame_rate = static_cast<float>(codec_.maxFramerate);
    configurations_[i].frame_dropping_on = codec_.H264()->frameDroppingOn;
    configurations_[i].key_frame_interval = codec_.H264()->keyFrameInterval;
	 
    x264_param_t x264_param = CreateEncoderParams(i);
    x264_t* x264_encoder = x264_encoder_open(&x264_param);
    if (x264_encoder == nullptr) {
      // Failed to create encoder.
      RTC_LOG(LS_ERROR) << "Failed to create X264 encoder";
      RTC_DCHECK(!x264_encoder);
      Release();
      ReportError();
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
#ifdef SAVE_ENCODEDE_FILE
    std::string file_name("x264svc_");
    file_name += std::to_string(encoder_seq_) + " " + std::to_string(i) + std::string(".264");
    std::ofstream ofile(file_name, std::ios::binary | std::ios::out);
    if (!ofile) {
      RTC_LOG(LS_INFO) << "failed to open file: " << file_name;
    } else
      output_files_.push_back(std::move(ofile));
#endif
    RTC_DCHECK(x264_encoder);
    encoders_[i] = x264_encoder;

    // Create downscaled image buffers.
    if (i > 0) {
      downscaled_buffers_[i - 1] = I420Buffer::Create(
          configurations_[i].width, configurations_[i].height,
          configurations_[i].width, configurations_[i].width / 2,
          configurations_[i].width / 2);
    }

    // Codec_settings uses kbits/second; encoder uses bits/second.
    configurations_[i].max_bps = codec_.maxBitrate * 1000;
    configurations_[i].target_bps = codec_.startBitrate * 1000;

    // Initialize encoded image. Default buffer size: size of unencoded data.

    const size_t new_capacity =
        CalcBufferSize(VideoType::kI420, codec_.simulcastStream[idx].width,
                       codec_.simulcastStream[idx].height);
    encoded_images_[i].Allocate(new_capacity);
    encoded_images_[i]._completeFrame = true;
    encoded_images_[i]._encodedWidth = codec_.simulcastStream[idx].width;
    encoded_images_[i]._encodedHeight = codec_.simulcastStream[idx].height;
    encoded_images_[i].set_size(0);
  }

  SimulcastRateAllocator init_allocator(codec_);
  VideoBitrateAllocation allocation = init_allocator.GetAllocation(
      codec_.startBitrate * 1000, codec_.maxFramerate);
  return SetRateAllocation(allocation, codec_.maxFramerate);
}

int32_t X264EncoderImpl::Release() {
  while (!encoders_.empty()) {
    x264_t* x264_encoder = encoders_.back();
    if (x264_encoder) {
      x264_encoder_close(x264_encoder);
    }
    encoders_.pop_back();
  }
  downscaled_buffers_.clear();
  configurations_.clear();
  encoded_images_.clear();
  pictures_.clear();
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t X264EncoderImpl::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  encoded_image_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t X264EncoderImpl::SetRateAllocation(
    const VideoBitrateAllocation& bitrate,
    uint32_t new_framerate) {
  if (encoders_.empty())
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;

  if (new_framerate < 1)
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;

  if (bitrate.get_sum_bps() == 0) {
    // Encoder paused, turn off all encoding.
    for (size_t i = 0; i < configurations_.size(); ++i)
      configurations_[i].SetStreamState(false);
    return WEBRTC_VIDEO_CODEC_OK;
  }

  // At this point, bitrate allocation should already match codec settings.
  if (codec_.maxBitrate > 0)
    RTC_DCHECK_LE(bitrate.get_sum_kbps(), codec_.maxBitrate);
  RTC_DCHECK_GE(bitrate.get_sum_kbps(), codec_.minBitrate);
  if (codec_.numberOfSimulcastStreams > 0)
    RTC_DCHECK_GE(bitrate.get_sum_kbps(), codec_.simulcastStream[0].minBitrate);

  codec_.maxFramerate = new_framerate;

  size_t stream_idx = encoders_.size() - 1;
  for (size_t i = 0; i < encoders_.size(); ++i, --stream_idx) {
    // Update layer config.
    configurations_[i].target_bps = bitrate.GetSpatialLayerSum(stream_idx);
    //if (configurations_[i].target_bps == 0)  // ---ylr
    //  configurations_[i].target_bps = 300000;
    configurations_[i].max_frame_rate = static_cast<float>(new_framerate);

    if (configurations_[i].target_bps) {
      configurations_[i].SetStreamState(true);

      // Update h264 encoder.
      if (encoders_[i]) {
        x264_param_t curparms;
        x264_encoder_parameters(encoders_[i], &curparms);
        curparms.i_fps_num = configurations_[i].max_frame_rate;
        curparms.i_fps_den = 1;

          curparms.rc.i_bitrate = configurations_[i].target_bps / 1000;
          curparms.rc.i_vbv_max_bitrate = configurations_[i].target_bps/1000;
          curparms.rc.i_vbv_buffer_size = configurations_[i].target_bps/1000;

        int retval = x264_encoder_reconfig(encoders_[i], &curparms);
        if (retval < 0)
          return WEBRTC_VIDEO_CODEC_ERROR;
      }
    } else {
      configurations_[i].SetStreamState(false);
    }
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t X264EncoderImpl::Encode(
    const VideoFrame& input_frame,
    const std::vector<VideoFrameType>* frame_types) {

	count_++;
	if (encoders_.empty()) {
    ReportError();
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (!encoded_image_callback_) {
    RTC_LOG(LS_WARNING)
        << "InitEncode() has been called, but a callback function "
        << "has not been set with RegisterEncodeCompleteCallback()";
    ReportError();
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  rtc::scoped_refptr<const I420BufferInterface> frame_buffer =
      input_frame.video_frame_buffer()->ToI420();

  bool send_key_frame = false;
  for (size_t i = 0; i < configurations_.size(); ++i) {
    if (configurations_[i].key_frame_request && configurations_[i].sending) {
      send_key_frame = true;
      break;
    }
  }
  if (!send_key_frame && frame_types) {
    for (size_t i = 0; i < frame_types->size() && i < configurations_.size();
         ++i) {
      if ((*frame_types)[i] == VideoFrameType::kVideoFrameKey &&
          configurations_[i].sending) {
        send_key_frame = true;
        break;
      }
    }
  }

  RTC_DCHECK_EQ(configurations_[0].width, frame_buffer->width());
  RTC_DCHECK_EQ(configurations_[0].height, frame_buffer->height());

  for (size_t i = 0; i < encoders_.size(); ++i) {
    // EncodeFrame input.
    // memset(&pictures_[i], 0, sizeof(x264_picture_t));
    x264_picture_init(&pictures_[i]);
    pictures_[i].i_type = X264_TYPE_AUTO;
    pictures_[i].i_qpplus1 = 0;
    // pictures_[i].i_pts = framenum_;
    pictures_[i].param = NULL;
    pictures_[i].img.i_csp = X264_CSP_I420;
    pictures_[i].img.i_plane = 3;

    // Downscale images on second and ongoing layers.
    if (i == 0) {
      pictures_[i].img.plane[0] = const_cast<uint8_t*>(frame_buffer->DataY());
      pictures_[i].img.plane[1] = const_cast<uint8_t*>(frame_buffer->DataU());
      pictures_[i].img.plane[2] = const_cast<uint8_t*>(frame_buffer->DataV());
      pictures_[i].img.plane[3] = 0;
      pictures_[i].img.i_stride[0] = frame_buffer->StrideY();
      pictures_[i].img.i_stride[1] = frame_buffer->StrideU();
      pictures_[i].img.i_stride[2] = frame_buffer->StrideV();
      pictures_[i].img.i_stride[3] = 0;
    } else {
      pictures_[i].img.i_stride[0] = downscaled_buffers_[i - 1]->StrideY();
      pictures_[i].img.i_stride[1] = downscaled_buffers_[i - 1]->StrideU();
      pictures_[i].img.i_stride[2] = downscaled_buffers_[i - 1]->StrideV();
      pictures_[i].img.plane[0] =
          const_cast<uint8_t*>(downscaled_buffers_[i - 1]->DataY());
      pictures_[i].img.plane[1] =
          const_cast<uint8_t*>(downscaled_buffers_[i - 1]->DataU());
      pictures_[i].img.plane[2] =
          const_cast<uint8_t*>(downscaled_buffers_[i - 1]->DataV());
      // Scale the image down a number of times by downsampling factor.
      libyuv::I420Scale(
          pictures_[i - 1].img.plane[0], pictures_[i - 1].img.i_stride[0],
          pictures_[i - 1].img.plane[1], pictures_[i - 1].img.i_stride[1],
          pictures_[i - 1].img.plane[2], pictures_[i - 1].img.i_stride[2],
          configurations_[i - 1].width, configurations_[i - 1].height,
          pictures_[i].img.plane[0], pictures_[i].img.i_stride[0],
          pictures_[i].img.plane[1], pictures_[i].img.i_stride[1],
          pictures_[i].img.plane[2], pictures_[i].img.i_stride[2],
          configurations_[i].width, configurations_[i].height,
          libyuv::kFilterBilinear);
    }

    if (!configurations_[i].sending) {
      RTC_LOG(LS_ERROR) << "not sending, encoder id: " << i;
      continue;
    }
    if (frame_types != nullptr) {
      // Skip frame?
      if ((*frame_types)[i] == VideoFrameType::kEmptyFrame) {
        RTC_LOG(LS_ERROR) << "skip frame, encoder id: " << i;
        continue;
      }
    }
    if (send_key_frame) {
      // API doc says ForceIntraFrame(false) does nothing, but calling this
      // function forces a key frame regardless of the |bIDR| argument's value.
      // (If every frame is a key frame we get lag/delays.)
      pictures_[i].i_type = X264_TYPE_IDR;
      configurations_[i].key_frame_request = false;
    }

    // EncodeFrame output.
    x264_nal_t* pnals_out = nullptr;
    x264_picture_t pic_out;
    int num_nals = 0;
    int enc_ret = -1; 
    if (frame_buffer->width() == 1920) {
      enc_ret = x264_encoder_encode(encoders_[i], &pnals_out, &num_nals,
                                        &pictures_[i], &pic_out);
    } else {
    
     enc_ret = x264_encoder_encode(encoders_[i], &pnals_out, &num_nals,
                                      &pictures_[i], &pic_out);
	}
    if (enc_ret <= 0) {
      RTC_LOG(LS_ERROR) << "X264 frame encoding failed, EncodeFrame returned "
                        << enc_ret << ".";
      ReportError();
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

  /*  RTC_LOG(LS_INFO) << "encoder id: " << i
                     << " encoders size: " << encoders_.size()
                     << " temporal_layers: " << num_temporal_layers_
                     << " time ms: " << rtc::TimeMillis()
                     << " count_: " << count_;*/

    int temporal_id = -1;
    for (int idx = 0; idx < num_nals; idx++) {
      int nal_unit_type = pnals_out[idx].i_type;
      int offset = pnals_out[idx].b_long_startcode ? 4 : 3;
      if (nal_unit_type == 14) {
        temporal_id = (pnals_out[idx].p_payload[offset + 1 + 2] & 0xE0) >> 5;
       /* RTC_LOG(LS_INFO) << "NAL type: " << pnals_out[idx + 1].i_type
                         << " temporal_id: " << temporal_id;*/
      }
    }

#ifdef SAVE_ENCODEDE_FILE
    if (output_files_[i]) {
      for (int idx = 0; idx < num_nals; idx++) {
        output_files_[i].write(
            reinterpret_cast<char*>(pnals_out[idx].p_payload),
            pnals_out[idx].i_payload);
      }
    }
    if (temporal_id != -1) {
      for (int tid = 0; tid < 4 - temporal_id; tid++) {
        for (int idx = 0; idx < num_nals; idx++) {
          output_temporal_files_[3 - tid].write(
              reinterpret_cast<char*>(pnals_out[idx].p_payload),
              pnals_out[idx].i_payload);
        }
      }
    }

#endif
    //  RTC_DCHECK_NE(temporal_id, -1);
    encoded_images_[i]._encodedWidth = configurations_[i].width;
    encoded_images_[i]._encodedHeight = configurations_[i].height;
    encoded_images_[i].SetTimestamp(input_frame.timestamp());
    encoded_images_[i].ntp_time_ms_ = input_frame.ntp_time_ms();
    encoded_images_[i].capture_time_ms_ = input_frame.render_time_ms();
    encoded_images_[i].rotation_ = input_frame.rotation();
    encoded_images_[i].SetColorSpace(input_frame.color_space());
    encoded_images_[i].content_type_ =
        (codec_.mode == VideoCodecMode::kScreensharing)
            ? VideoContentType::SCREENSHARE
            : VideoContentType::UNSPECIFIED;
    encoded_images_[i].timing_.flags = VideoSendTiming::kInvalid;
    encoded_images_[i]._frameType = ConvertToVideoFrameType(pic_out.i_type);
    encoded_images_[i].SetSpatialIndex(configurations_[i].simulcast_idx);

    // Split encoded image up into fragments. This also updates
    // |encoded_image_|.
    RTPFragmentationHeader frag_header;
    frag_header.VerifyAndAllocateFragmentationHeader(num_nals);
    RtpFragmentize(&encoded_images_[i], *frame_buffer, pnals_out, num_nals,
                   &frag_header);

    // Deliver encoded image.
    CodecSpecificInfo codec_specific;
    codec_specific.codecType = kVideoCodecH264;
    codec_specific.codecSpecific.H264.packetization_mode = packetization_mode_;
    codec_specific.codecSpecific.H264.temporal_idx = kNoTemporalIdx;
    codec_specific.codecSpecific.H264.idr_frame = pic_out.b_keyframe;
    codec_specific.codecSpecific.H264.base_layer_sync = false;

    if (num_temporal_layers_ > 1) {
      const uint8_t tid = temporal_id;
      codec_specific.codecSpecific.H264.temporal_idx = tid;
      codec_specific.codecSpecific.H264.base_layer_sync =
          tid > 0 && tid < tl0sync_limit_;
      if (codec_specific.codecSpecific.H264.base_layer_sync) {
        tl0sync_limit_ = tid;
      }
      if (tid == 0) {
        tl0sync_limit_ = num_temporal_layers_;
      }

     /* RTC_LOG(LS_INFO) << "---ylr x264 tid: " << tid << " base_layer_sync: "
                       << codec_specific.codecSpecific.H264.base_layer_sync;*/
    }

    // Parse QP.
    h264_bitstream_parser_.ParseBitstream(encoded_images_[i].data(),
                                          encoded_images_[i].size());
    h264_bitstream_parser_.GetLastSliceQp(&encoded_images_[i].qp_);

    encoded_image_callback_->OnEncodedImage(encoded_images_[i], &codec_specific,
                                            &frag_header);
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

VideoEncoder::EncoderInfo X264EncoderImpl::GetEncoderInfo() const {
  EncoderInfo info;
  info.supports_native_handle = false;
  info.implementation_name = "X264";
  info.scaling_settings =
      VideoEncoder::ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
  info.is_hardware_accelerated = false;
  info.has_internal_source = false;
  return info;
}
void X264EncoderImpl::X264Log(void* handle,
                              int i_level,
                              const char* fmt,
                              va_list vars) {
  char buffer[1000];
  int size = vsprintf(buffer, fmt, vars);
  RTC_LOG(LS_INFO) << "i_level: " << i_level << " info size: " << size
                   << " info: " << buffer << " time ms: " << rtc::TimeMillis();
}
x264_param_t X264EncoderImpl::CreateEncoderParams(size_t i) const {
  x264_param_t param;
  x264_param_t* p_params = &param;
  x264_param_default_preset(p_params, x264_preset_names[2], "zerolatency");
  int idx = (codec_.numberOfSimulcastStreams - 1) - i;
  RTC_DCHECK_GE(idx, 0);
  if (codec_.mode == VideoCodecMode::kRealtimeVideo ||
      codec_.mode == VideoCodecMode::kScreensharing) {
    p_params->i_width = configurations_[i].width;
    p_params->i_height = configurations_[i].height;
    p_params->i_fps_num = configurations_[i].max_frame_rate;
    p_params->i_fps_den = 1;
    p_params->i_keyint_max = configurations_[i].key_frame_interval;
    p_params->b_annexb = 1;
    p_params->rc.i_rc_method = X264_RC_ABR;
    p_params->rc.i_bitrate = codec_.simulcastStream[idx].targetBitrate;
    p_params->rc.i_vbv_buffer_size = 20000;
    //codec_.simulcastStream[idx].targetBitrate;  // 8000
	p_params->rc.i_vbv_max_bitrate = codec_.simulcastStream[idx].targetBitrate;

    //p_params->rc.b_filler = 1;
    //p_params->b_vfr_input = 0;
    //p_params->pf_log = X264Log;
    //p_params->i_log_level = X264_LOG_DEBUG;
//#ifdef RLCLOUD
#ifdef _RLCLOUD
  //  p_params->iTemporalLayers = num_temporal_layers_;
	//LZM------------
    if (codec_.mode == VideoCodecMode::kScreensharing) {
      p_params->iTemporalLayers = 1;
      p_params->bScreenMode = true;
    } else {
      p_params->bScreenMode = false;
      p_params->iTemporalLayers = 1;
	}
    
#endif
  }

  return param;
}

void X264EncoderImpl::ReportInit() {
  if (has_reported_init_)
    return;
  has_reported_init_ = true;
}

void X264EncoderImpl::ReportError() {
  if (has_reported_error_)
    return;
  has_reported_error_ = true;
}

void X264EncoderImpl::LayerConfig::SetStreamState(bool send_stream) {
  if (send_stream && !sending) {
    // Need a key frame if we have not sent this stream before.
    key_frame_request = true;
  }
  sending = send_stream;
}
}  // namespace webrtc
