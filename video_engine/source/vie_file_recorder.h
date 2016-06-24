/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_RECORDER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_RECORDER_H_

#include "file_recorder.h"
#include "typedefs.h"
#include "vie_file.h"
#include "voe_file.h"

namespace cloopenwebrtc {

class CriticalSectionWrapper;

class ViEFileRecorder : protected OutStream {
 public:
  explicit ViEFileRecorder(int channel_id);
  ~ViEFileRecorder();

  int StartRecording(const char* file_nameUTF8,
                     const VideoCodec& codec_inst,
                     AudioSource audio_source, int audio_channel,
                     const CodecInst& audio_codec_inst,
                     VoiceEngine* voe_ptr,
                     const FileFormats file_format = kFileFormatAviFile);
  int StopRecording();

  void SetFrameDelay(int frame_delay);
  bool RecordingStarted();

  // Records incoming decoded video frame to file.
  void RecordVideoFrame(const I420VideoFrame& video_frame);

 protected:
  bool FirstFrameRecorded();
  bool IsRecordingFileFormat(const FileFormats file_format);

  // Implements OutStream.
  bool Write(const void* buf, size_t len);
  int Rewind();

 private:
  CriticalSectionWrapper* recorder_cs_;

  FileRecorder* file_recorder_;
  bool is_first_frame_recorded_;
  bool is_out_stream_started_;
  int instance_id_;
  int frame_delay_;
  int audio_channel_;
  AudioSource audio_source_;
  VoEFile* voe_file_interface_;
};

}  // namespace cloopenwebrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_FILE_RECORDER_H_
