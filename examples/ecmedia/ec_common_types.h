#ifndef EC_COMMON_TYPES_H
#define EC_COMMON_TYPES_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace ecmedia {

/**********************codec***********************/
const char kParamValueEmpty[] = "";
typedef std::map<std::string, std::string> CodecParameterMap;

struct FeedbackParam {
 public:
  // FeedbackParam() = default;
  FeedbackParam(const std::string& id, const std::string& param)
      : id_(id), param_(param) {}
  ~FeedbackParam() {}
  explicit FeedbackParam(const std::string& id)
      : id_(id), param_(kParamValueEmpty) {}

  bool operator==(const FeedbackParam& other) const;

  const std::string& id() const { return id_; }
  const std::string& param() const { return param_; }

 private:
  std::string id_;     // e.g. "nack", "ccm"
  std::string param_;  // e.g. "", "rpsi", "fir"
};

struct FeedbackParams {
 public:
  FeedbackParams() {}
  ~FeedbackParams() {}
  bool operator==(const FeedbackParams& other) const;

  bool Has(const FeedbackParam& param) const;
  void Add(const FeedbackParam& param);

  void Intersect(const FeedbackParams& from);

  const std::vector<FeedbackParam>& params() const { return params_; }

 private:
  bool HasDuplicateEntries() const;

  std::vector<FeedbackParam> params_;
};

struct Codec {
  int id;
  std::string name;
  int clockrate;
  CodecParameterMap params;
  FeedbackParams feedback_params;

  virtual ~Codec() {}

  // Indicates if this codec is compatible with the specified codec.
  bool Matches(const Codec& codec) const;
  // bool MatchesCapability(const webrtc::RtpCodecCapability& capability) const;

  // Find the parameter for |name| and write the value to |out|.
  bool GetParam(const std::string& name, std::string* out) const;
  bool GetParam(const std::string& name, int* out) const;

  void SetParam(const std::string& name, const std::string& value);
  void SetParam(const std::string& name, int value);

  // It is safe to input a non-existent parameter.
  // Returns true if the parameter existed, false if it did not exist.
  bool RemoveParam(const std::string& name);

  bool HasFeedbackParam(const FeedbackParam& param) const;
  void AddFeedbackParam(const FeedbackParam& param);

  // Filter |this| feedbacks params such that only those shared by both |this|
  // and |other| are kept.
  void IntersectFeedbackParams(const Codec& other);

  // virtual webrtc::RtpCodecParameters ToCodecParameters() const;

  /*Codec& operator=(const Codec& c);
  Codec& operator=(Codec&& c);*/

  bool operator==(const Codec& c) const;

  bool operator!=(const Codec& c) const { return !(*this == c); }

 protected:
  // A Codec can't be created without a subclass.
  // Creates a codec with the given parameters.
  Codec(int id, const std::string& name, int clockrate);
  // Creates an empty codec.
  Codec() : id(0), clockrate(0){}
  // Codec(const Codec& c);
  // Codec(Codec&& c);
};

struct AudioCodec : public Codec {
  int bitrate;
  size_t channels;

  // Creates a codec with the given parameters.
  AudioCodec(int id,
             const std::string& name,
             int clockrate,
             int bitrate,
             size_t channels);
  // Creates an empty codec.
  AudioCodec() : Codec(), bitrate(0), channels(0){}

  /* AudioCodec(const AudioCodec& c);
   AudioCodec(AudioCodec&& c);*/

  ~AudioCodec() override {}

  // Indicates if this codec is compatible with the specified codec.
  bool Matches(const AudioCodec& codec) const;

  std::string ToString() const;

  // webrtc::RtpCodecParameters ToCodecParameters() const override;

  /*AudioCodec& operator=(const AudioCodec& c);
  AudioCodec& operator=(AudioCodec&& c);*/

  bool operator==(const AudioCodec& c) const;

  bool operator!=(const AudioCodec& c) const { return !(*this == c); }
};

struct VideoCodec : public Codec {
  std::string packetization;

  // Creates a codec with the given parameters.
  VideoCodec(int id, const std::string& name);
  // Creates a codec with the given name and empty id.
  explicit VideoCodec(const std::string& name);
  // Creates an empty codec.
  VideoCodec();
  // VideoCodec(const VideoCodec& c);

  // explicit VideoCodec(const webrtc::SdpVideoFormat& c);

  // VideoCodec(VideoCodec&& c);

  ~VideoCodec() override {}

  // Indicates if this video codec is the same as the other video codec, e.g. if
  // they are both VP8 or VP9, or if they are both H264 with the same H264
  // profile. H264 levels however are not compared.
  bool Matches(const VideoCodec& codec) const;

  std::string ToString() const;

  // webrtc::RtpCodecParameters ToCodecParameters() const override;

  /*VideoCodec& operator=(const VideoCodec& c);
  VideoCodec& operator=(VideoCodec&& c);*/

  bool operator==(const VideoCodec& c) const;

  bool operator!=(const VideoCodec& c) const { return !(*this == c); }

  // Return packetization which both |local_codec| and |remote_codec| support.
  static std::string IntersectPacketization(const VideoCodec& local_codec,
                                            const VideoCodec& remote_codec);

  static VideoCodec CreateRtxCodec(int rtx_payload_type,
                                   int associated_payload_type);

  enum CodecType {
    CODEC_VIDEO,
    CODEC_RED,
    CODEC_ULPFEC,
    CODEC_FLEXFEC,
    CODEC_RTX,
  };

  CodecType GetCodecType() const;
  // Validates a VideoCodec's payload type, dimensions and bitrates etc. If they
  // don't make sense (such as max < min bitrate), and error is logged and
  // ValidateCodecFormat returns false.
  bool ValidateCodecFormat() const;

 private:
  void SetDefaultParameters();
};

}  // namespace ecmedia

#endif  //