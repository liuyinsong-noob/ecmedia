/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_IMPL_H_

#include "../system_wrappers/include/scoped_ptr.h"
#include "typedefs.h"
#include "vie_file.h"
#include "vie_defines.h"
#include "vie_frame_provider_base.h"
#include "vie_ref_count.h"
#include "vie_shared_data.h"
#include "video_coding_defines.h"

namespace cloopenwebrtc {

class ConditionVariableWrapper;
class CriticalSectionWrapper;
class ViESharedData;

class ViECaptureSnapshot : public ViEFrameCallback {
 public:
  ViECaptureSnapshot();
  ~ViECaptureSnapshot();


  virtual int GetClassNameType();

  bool GetSnapshot(I420VideoFrame& video_frame, unsigned int max_wait_time);

  // Implements ViEFrameCallback.
  virtual void DeliverFrame(int id, I420VideoFrame* video_frame,
                            const std::vector<uint32_t>& csrcs);
  virtual void DelayChanged(int id, int frame_delay) {}
  virtual int GetPreferedFrameSettings(int* width, int* height,
                                       int* frame_rate) {
    return -1;
  }
  virtual void ProviderDestroyed(int id) {}

 private:
  scoped_ptr<CriticalSectionWrapper> crit_;
  scoped_ptr<ConditionVariableWrapper> condition_varaible_;
  I420VideoFrame* video_frame_;
};

class ViEFileImpl
    : public ViEFile,
      public ViERefCount {
 public:
  // Implements ViEFile.
  virtual int Release();
  virtual int StartPlayFile(const char* file_nameUTF8, int& file_id,
                            const bool loop = false,
                            const FileFormats file_format = kFileFormatAviFile);
  virtual int StopPlayFile(const int file_id);
  virtual int RegisterObserver(int file_id, ViEFileObserver& observer);
  virtual int DeregisterObserver(int file_id, ViEFileObserver& observer);
  virtual int SendFileOnChannel(const int file_id, const int video_channel);
  virtual int StopSendFileOnChannel(const int video_channel);
  virtual int StartPlayFileAsMicrophone(const int file_id,
                                        const int audio_channel,
                                        bool mix_microphone = false,
                                        float volume_scaling = 1);
  virtual int StopPlayFileAsMicrophone(const int file_id,
                                       const int audio_channel);
  virtual int StartPlayAudioLocally(const int file_id, const int audio_channel,
                                    float volume_scaling = 1);
  virtual int StopPlayAudioLocally(const int file_id, const int audio_channel);

  virtual void RegisterVideoFrameStorageCallBack(const int video_channel, VCMFrameStorageCallback *callback);

  virtual int StartRecordOutgoingVideo(
      const int video_channel,
      const char* file_nameUTF8,
      AudioSource audio_source,
      const CodecInst& audio_codec,
      const VideoCodec& video_codec,
      const FileFormats file_format = kFileFormatAviFile);
  virtual int StartRecordIncomingVideo(
      const int video_channel,
      const char* file_nameUTF8,
      AudioSource audio_source,
      const CodecInst& audio_codec,
      const VideoCodec& video_codec,
      const FileFormats file_format = kFileFormatAviFile);
  virtual int StopRecordOutgoingVideo(const int video_channel);
  virtual int StopRecordIncomingVideo(const int video_channel);
  virtual int GetFileInformation(
      const char* file_name,
      VideoCodec& video_codec,
      CodecInst& audio_codec,
      const FileFormats file_format = kFileFormatAviFile);
  virtual int GetRenderSnapshot(const int video_channel,
                                const char* file_nameUTF8);
  virtual int GetRenderSnapshot(const int video_channel,
                                    ViEPicture& picture, RawVideoType type);
  virtual int FreePicture(ViEPicture& picture);
  virtual int GetCaptureDeviceSnapshot(const int capture_id,
                                       const char* file_nameUTF8);
  virtual int GetCaptureDeviceSnapshot(const int capture_id,
                                       ViEPicture& picture, RawVideoType type);
  virtual int SetCaptureDeviceImage(const int capture_id,
                                    const char* file_nameUTF8);
  virtual int SetCaptureDeviceImage(const int capture_id,
                                    const ViEPicture& picture);
  virtual int SetRenderStartImage(const int video_channel,
                                  const char* file_nameUTF8);
  virtual int SetRenderStartImage(const int video_channel,
                                  const ViEPicture& picture);
  virtual int SetRenderTimeoutImage(const int video_channel,
                                    const char* file_nameUTF8,
                                    const unsigned int timeout_ms);
  virtual int SetRenderTimeoutImage(const int video_channel,
                                    const ViEPicture& picture,
                                    const unsigned int timeout_ms);

 protected:
  ViEFileImpl(ViESharedData* shared_data);
  virtual ~ViEFileImpl();

 private:
  WebRtc_Word32 GetNextCapturedFrame(WebRtc_Word32 capture_id,
                                     I420VideoFrame& video_frame);

  ViESharedData* shared_data_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_FILE_IMPL_H_
