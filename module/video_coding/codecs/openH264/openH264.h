#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_H_

#include "video_codec_interface.h"

namespace cloopenwebrtc {

class OpenH264Encoder : public VideoEncoder {
 public:
  static OpenH264Encoder* Create();

  virtual ~OpenH264Encoder() {};
};  // end of H264Encoder class


class OpenH264Decoder : public VideoDecoder {
 public:
  static OpenH264Decoder* Create();

  virtual ~OpenH264Decoder() {};
};  // end of H264Decoder class
}  // namespace webrtc

#endif // WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_
