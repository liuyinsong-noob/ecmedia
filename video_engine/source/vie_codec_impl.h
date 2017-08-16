/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_CODEC_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CODEC_IMPL_H_

#include "typedefs.h"
#include "vie_codec.h"
#include "vie_defines.h"
#include "vie_ref_count.h"

namespace cloopenwebrtc {

class ViESharedData;

class ViECodecImpl
    : public ViECodec,
      public ViERefCount {
 public:
  virtual int Release();

  // Implements ViECodec.
  virtual int NumberOfCodecs() const;
  virtual int GetCodec(const unsigned char list_number,
                       VideoCodec& video_codec) const;
  virtual int SetSendCodec(const int video_channel,
                           VideoCodec& video_codec);
  virtual int GetSendCodec(const int video_channel,
                           VideoCodec& video_codec) const;
  virtual int SetReceiveCodec(const int video_channel,
                              const VideoCodec& video_codec);
  virtual int GetReceiveCodec(const int video_channel,
                              VideoCodec& video_codec) const;
  virtual int GetCodecConfigParameters(
    const int video_channel,
    unsigned char config_parameters[kConfigParameterSize],
    unsigned char& config_parameters_size) const;
  virtual int SetImageScaleStatus(const int video_channel, const bool enable);
  virtual int GetSendCodecStastistics(const int video_channel,
                                      unsigned int& key_frames,
                                      unsigned int& delta_frames) const;
  virtual int GetReceiveCodecStastistics(const int video_channel,
                                         unsigned int& key_frames,
                                         unsigned int& delta_frames) const;
  virtual int GetReceiveSideDelay(const int video_channel,
                                  int* delay_ms) const;
  virtual int GetCodecTargetBitrate(const int video_channel,
                                    unsigned int* bitrate) const;
  virtual int GetNumDiscardedPackets(int video_channel) const;
  virtual int SetKeyFrameRequestCallbackStatus(const int video_channel,
                                               const bool enable);
  virtual int SetSignalKeyPacketLossStatus(const int video_channel,
                                           const bool enable,
                                           const bool only_key_frames = false);
  virtual int RegisterEncoderObserver(const int video_channel,
                                      ViEEncoderObserver& observer);
  virtual int DeregisterEncoderObserver(const int video_channel);
  virtual int RegisterDecoderObserver(const int video_channel,
                                      ViEDecoderObserver& observer);
  virtual int DeregisterDecoderObserver(const int video_channel);
  virtual int SendKeyFrame(const int video_channel);
  virtual int WaitForFirstKeyFrame(const int video_channel, const bool wait);
  virtual int StartDebugRecording(int video_channel,
                                  const char* file_name_utf8);
  virtual int StopDebugRecording(int video_channel);
  virtual void SuspendBelowMinBitrate(int video_channel);
  virtual bool GetSendSideDelay(int video_channel, int* avg_delay_ms,
                                int* max_delay_ms) const;

  virtual int SetKeyFrameRequestCb(const int video_channel, bool isVideoConf,onEcMediaRequestKeyFrame cb);
  virtual int RegisterCaptureObserver(const int video_channel, void *capture, const int capture_id);
  virtual int DeRegisterCaptureObserver(const int video_channel);
  virtual int SetFrameScaleType(const int video_channel, FrameScaleType frame_scale_type);
  virtual int AddI420FrameCallback(const int video_channel, ECMedia_I420FrameCallBack callback);
  virtual int SetVideoSendQmMode(int channel_id, int mode);
          
 protected:
  explicit ViECodecImpl(ViESharedData* shared_data);
  virtual ~ViECodecImpl();

 private:
  bool CodecValid(const VideoCodec& video_codec);

  ViESharedData* shared_data_;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_CODEC_IMPL_H_
