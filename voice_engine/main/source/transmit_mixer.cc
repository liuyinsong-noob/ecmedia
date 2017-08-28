/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "transmit_mixer.h"

#include "audio_frame_operations.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "logging.h"
#include "trace.h"
#include "channel.h"
#include "channel_manager.h"
#include "voe_external_media.h"
#include "statistics.h"
#include "utility.h"
#include "voe_base_impl.h"
#include "wavfile.h"



#define RECORD_AUDIO_TEST 0

#define RECORD_AUDIO_TEST_2 0
#if RECORD_AUDIO_TEST_2 
#include <Shlobj.h>
#endif

char *filename_path = NULL;

#define WEBRTC_ABS(a) (((a) < 0) ? -(a) : (a))

namespace cloopenwebrtc {
namespace voe {

// TODO(ajm): The thread safety of this is dubious...
void
TransmitMixer::OnPeriodicProcess()
{
//    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
//                 "TransmitMixer::OnPeriodicProcess()");

#if defined(WEBRTC_VOICE_ENGINE_TYPING_DETECTION)
    if (_typingNoiseWarningPending)
    {
        CriticalSectionScoped cs(&_callbackCritSect);
        if (_voiceEngineObserverPtr)
        {
            if (_typingNoiseDetected) {
                WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                             "TransmitMixer::OnPeriodicProcess() => "
                             "CallbackOnError(VE_TYPING_NOISE_WARNING)");
                _voiceEngineObserverPtr->CallbackOnError(
                    -1,
                    VE_TYPING_NOISE_WARNING);
            } else {
                WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                             "TransmitMixer::OnPeriodicProcess() => "
                             "CallbackOnError(VE_TYPING_NOISE_OFF_WARNING)");
                _voiceEngineObserverPtr->CallbackOnError(
                    -1,
                    VE_TYPING_NOISE_OFF_WARNING);
            }
        }
        _typingNoiseWarningPending = false;
    }
#endif

    bool saturationWarning = false;
    {
      // Modify |_saturationWarning| under lock to avoid conflict with write op
      // in ProcessAudio and also ensure that we don't hold the lock during the
      // callback.
      CriticalSectionScoped cs(&_critSect);
      saturationWarning = _saturationWarning;
      if (_saturationWarning)
        _saturationWarning = false;
    }

    if (saturationWarning)
    {
        CriticalSectionScoped cs(&_callbackCritSect);
        if (_voiceEngineObserverPtr)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                         "TransmitMixer::OnPeriodicProcess() =>"
                         " CallbackOnError(VE_SATURATION_WARNING)");
            _voiceEngineObserverPtr->CallbackOnError(-1, VE_SATURATION_WARNING);
        }
    }
}


void TransmitMixer::PlayNotification(int32_t id,
                                     uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::PlayNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void TransmitMixer::RecordNotification(int32_t id,
                                       uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "TransmitMixer::RecordNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void TransmitMixer::PlayFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::PlayFileEnded(id=%d)", id);

    assert(id == _filePlayerId);

    CriticalSectionScoped cs(&_critSect);

    _filePlaying = false;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::PlayFileEnded() =>"
                 "file player module is shutdown");
}

void
TransmitMixer::RecordFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::RecordFileEnded(id=%d)", id);

    if (id == _fileRecorderId)
    {
        CriticalSectionScoped cs(&_critSect);
        _fileRecording = false;
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId, -1),
                     "TransmitMixer::RecordFileEnded() => fileRecorder module"
                     "is shutdown");
    } else if (id == _fileCallRecorderId)
    {
        CriticalSectionScoped cs(&_critSect);
        _fileCallRecording = false;
        WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId, -1),
                     "TransmitMixer::RecordFileEnded() => fileCallRecorder"
                     "module is shutdown");
    }
}

int32_t
TransmitMixer::Create(TransmitMixer*& mixer, uint32_t instanceId)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(instanceId, -1),
                 "TransmitMixer::Create(instanceId=%d)", instanceId);
    mixer = new TransmitMixer(instanceId);
    if (mixer == NULL)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(instanceId, -1),
                     "TransmitMixer::Create() unable to allocate memory"
                     "for mixer");
        return -1;
    }
    return 0;
}

void
TransmitMixer::Destroy(TransmitMixer*& mixer)
{
    if (mixer)
    {
        delete mixer;
        mixer = NULL;
    }
}

TransmitMixer::TransmitMixer(uint32_t instanceId) :
    _engineStatisticsPtr(NULL),
    _channelManagerPtr(NULL),
    audioproc_(NULL),
    _voiceEngineObserverPtr(NULL),
    _processThreadPtr(NULL),
    _filePlayerPtr(NULL),
    _fileRecorderPtr(NULL),
    _fileCallRecorderPtr(NULL),
    // Avoid conflict with other channels by adding 1024 - 1026,
    // won't use as much as 1024 channels.
    _filePlayerId(instanceId + 1024),
    _fileRecorderId(instanceId + 1025),
    _fileCallRecorderId(instanceId + 1026),
    _filePlaying(false),
    _fileRecording(false),
    _fileCallRecording(false),
    _audioLevel(),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _callbackCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    _typingNoiseWarningPending(false),
    _typingNoiseDetected(false),
#endif
    _saturationWarning(false),
    _instanceId(instanceId),
    _mixFileWithMicrophone(false),
    _captureLevel(0),
    external_postproc_ptr_(NULL),
    external_preproc_ptr_(NULL),
    _mute(false),
    _remainingMuteMicTimeMs(0),
    stereo_codec_(false),
    swap_stereo_channels_(false),
    _fileindex(1)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::TransmitMixer() - ctor");
	 _fileopen   = false;
	 _writecount = 0;
	 _fileRecording = false;
}

TransmitMixer::~TransmitMixer()
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::~TransmitMixer() - dtor");
    _monitorModule.DeRegisterObserver();
    if (_processThreadPtr)
    {
        _processThreadPtr->DeRegisterModule(&_monitorModule);
    }
    DeRegisterExternalMediaProcessing(kRecordingAllChannelsMixed);
    DeRegisterExternalMediaProcessing(kRecordingPreprocessing);
    {
        CriticalSectionScoped cs(&_critSect);
        if (_fileRecorderPtr)
        {
            _fileRecorderPtr->RegisterModuleFileCallback(NULL);
            _fileRecorderPtr->StopRecording();
            FileRecorder::DestroyFileRecorder(_fileRecorderPtr);
            _fileRecorderPtr = NULL;
        }
        if (_fileCallRecorderPtr)
        {
            _fileCallRecorderPtr->RegisterModuleFileCallback(NULL);
            _fileCallRecorderPtr->StopRecording();
            FileRecorder::DestroyFileRecorder(_fileCallRecorderPtr);
            _fileCallRecorderPtr = NULL;
        }
        if (_filePlayerPtr)
        {
            _filePlayerPtr->RegisterModuleFileCallback(NULL);
            _filePlayerPtr->StopPlayingFile();
            FilePlayer::DestroyFilePlayer(_filePlayerPtr);
            _filePlayerPtr = NULL;
        }
    }
    delete &_critSect;
    delete &_callbackCritSect;

}

int32_t
TransmitMixer::SetEngineInformation(ProcessThread& processThread,
                                    Statistics& engineStatistics,
                                    ChannelManager& channelManager)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::SetEngineInformation()");

    _processThreadPtr = &processThread;
    _engineStatisticsPtr = &engineStatistics;
    _channelManagerPtr = &channelManager;

    if (_processThreadPtr->RegisterModule(&_monitorModule) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "TransmitMixer::SetEngineInformation() failed to"
                     "register the monitor module");
    } else
    {
        _monitorModule.RegisterObserver(*this);
    }

    return 0;
}

int32_t
TransmitMixer::RegisterVoiceEngineObserver(VoiceEngineObserver& observer)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::RegisterVoiceEngineObserver()");
    CriticalSectionScoped cs(&_callbackCritSect);

    if (_voiceEngineObserverPtr)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceError,
            "RegisterVoiceEngineObserver() observer already enabled");
        return -1;
    }
    _voiceEngineObserverPtr = &observer;
    return 0;
}

int32_t
TransmitMixer::SetAudioProcessingModule(AudioProcessing* audioProcessingModule)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::SetAudioProcessingModule("
                 "audioProcessingModule=0x%x)",
                 audioProcessingModule);
    audioproc_ = audioProcessingModule;
    return 0;
}

void TransmitMixer::GetSendCodecInfo(int* max_sample_rate, int* max_channels) {
  *max_sample_rate = 8000;
  *max_channels = 1;
  for (ChannelManager::Iterator it(_channelManagerPtr); it.IsValid();
       it.Increment()) {
    Channel* channel = it.GetChannel();
    if (channel->Sending()) {
      CodecInst codec;
      channel->GetSendCodec(codec);
      *max_sample_rate = std::max(*max_sample_rate, codec.plfreq);
      *max_channels = std::max(*max_channels, codec.channels);
    }
  }
}
    
    int32_t TransmitMixer::RecordAudioToFileCall(uint32_t mixingFrequency)
    {
        CriticalSectionScoped cs(&_critSect);
        if (_fileCallRecorderPtr == NULL)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                         "TransmitMixer::RecordAudioToFileCall() filerecorder doesnot"
                         "exist");
            return -1;
        }
        
        if (_fileCallRecorderPtr->RecordAudioToFile(_audioFrame) != 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                         "TransmitMixer::RecordAudioToFileCall() file recording"
                         "failed");
            return -1;
        }
        
        return 0;
    }

int32_t
TransmitMixer::PrepareDemux(const void* audioSamples,
                            uint32_t nSamples,
                            uint8_t nChannels,
                            uint32_t samplesPerSec,
                            uint16_t totalDelayMS,
                            int32_t clockDrift,
                            uint16_t currentMicLevel,
                            bool keyPressed)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::PrepareDemux(nSamples=%u, nChannels=%u,"
                 "samplesPerSec=%u, totalDelayMS=%u, clockDrift=%d,"
                 "currentMicLevel=%u)", nSamples, nChannels, samplesPerSec,
                 totalDelayMS, clockDrift, currentMicLevel);

    // --- Resample input audio and create/store the initial audio frame
    GenerateAudioFrame(static_cast<const int16_t*>(audioSamples),
                       nSamples,
                       nChannels,
                       samplesPerSec);
	    
#if RECORD_AUDIO_TEST_2
	{
		if (!_recordCaptureFile)
		{
			WCHAR appDir[MAX_PATH];
			SHGetSpecialFolderPath(NULL, (LPWSTR)appDir, CSIDL_LOCAL_APPDATA, 0);

			int textlen = WideCharToMultiByte(CP_UTF8, 0, appDir, -1, NULL, 0, NULL, NULL);
			char *result = (char *)malloc((textlen + 1)*sizeof(char));
			memset(result, 0, sizeof(char) * (textlen + 1));
			WideCharToMultiByte(CP_UTF8, 0, appDir, -1, result, textlen, NULL, NULL);

			char file_name_r[MAX_PATH];
			sprintf(file_name_r, "%s/CaptureAudioData.pcm", result);

			_recordCaptureFile = fopen(file_name_r, "wb");
		}

		if (_recordCaptureFile)
		{
			fwrite(_audioFrame.data_, 2, _audioFrame.samples_per_channel_, _recordCaptureFile);
			fflush(_recordCaptureFile);
		}
	}


#endif
//    char filename_r[256] = "e:105.wav";
#if RECORD_AUDIO_TEST
    //
    
    char filename_r[256]      = {0};
    char filename_record[256] = {0};
    char filename_proc[256]   = {0};
    if( !_fileopen && filename_path)
    {
        if(_fileindex > 20 )
            _fileindex = 0;
        if( _audioFrame.sample_rate_hz_ == 8000 )
        {
            sprintf(filename_r, "%s/itu_mos_test_squence/list@8k/%d_src.wav", filename_path, _fileindex);
            sprintf(filename_record, "%s/itu_mos_test_squence/list@8k/%d_record.pcm", filename_path, _fileindex);
            sprintf(filename_proc, "%s/itu_mos_test_squence/list@8k/%d_proc.pcm", filename_path, _fileindex);
        }
        else if( _audioFrame.sample_rate_hz_ == 16000 )
        {
            sprintf(filename_r, "%s/itu_mos_test_squence/list@16k/%d_src.wav", filename_path, _fileindex);
            sprintf(filename_record, "%s/itu_mos_test_squence/list@16k/%d_record.pcm", filename_path, _fileindex);
            sprintf(filename_proc, "%s/itu_mos_test_squence/list@16k/%d_proc.pcm", filename_path, _fileindex);
        }
#if 0
        if( OpenWavFile(&_wavParams, filename_r, FILE_READ) != -1 && WavFileReadHeader(&_wavParams) == 0 )
        {
            _fileopen = true;
//            _fileindex++;
        }
#endif
    }
    
#if 0
    if( _fileopen )
    {
        {
            int samples = _audioFrame.samples_per_channel_;
            CriticalSectionScoped cs2(&_callbackCritSect);
            int16_t in_data[160];
            int ww = WavFileRead( &_wavParams, in_data, samples*2 );
            if ( ww == samples*2 )
            {
                memcpy( _audioFrame.data_, in_data, samples*2 );
            }
            else
            {
                for(int i = 0; i < 80; ++i)
                    _audioFrame.data_[i] = 32767;
                
                memset(_audioFrame.data_ + 80, 0, (samples - 80)*2);
                WavFileClose(&_wavParams);
                _fileopen = false;
            }
        }
    }
#endif
    //end of added by zenggq
#endif
    
    {
      CriticalSectionScoped cs(&_callbackCritSect);
      if (external_preproc_ptr_) {
        external_preproc_ptr_->Process(-1, kRecordingPreprocessing,
                                       _audioFrame.data_,
                                       _audioFrame.samples_per_channel_,
                                       _audioFrame.sample_rate_hz_,
                                       _audioFrame.num_channels_ == 2);
      }
    }

	//    sean add begin 20140708 original audio sample
	_audioFrame2Up.CopyFrom(_audioFrame);
	//    sean add end 20140708 original audio sample
#if RECORD_AUDIO_TEST
    CodecInst codecInst = {100, "L16", _audioFrame.sample_rate_hz_, _audioFrame.sample_rate_hz_/100, 1, _audioFrame.sample_rate_hz_*16};
#if 0
    _fileopen = true;
    if( _fileopen )
    {
        StartRecordingMicrophone(filename_proc, &codecInst);
        StartRecordingCall(filename_record, &codecInst);
    }
    else
    {
        _writecount = 0;
        StopRecordingMicrophone();
        StopRecordingCall();
    }
#else
    StartRecordingMicrophone(filename_proc, &codecInst);
    StartRecordingCall(filename_record, &codecInst);
#endif
#endif

    // --- Near-end audio processing.
    ProcessAudio(totalDelayMS, clockDrift, currentMicLevel, keyPressed);

#if RECORD_AUDIO_TEST_2
	{
		if (!_recordProcesedFile)
		{
			WCHAR appDir[MAX_PATH];
			SHGetSpecialFolderPath(NULL, (LPWSTR)appDir, CSIDL_LOCAL_APPDATA, 0);

			int textlen = WideCharToMultiByte(CP_UTF8, 0, appDir, -1, NULL, 0, NULL, NULL);
			char *result = (char *)malloc((textlen + 1)*sizeof(char));
			memset(result, 0, sizeof(char) * (textlen + 1));
			WideCharToMultiByte(CP_UTF8, 0, appDir, -1, result, textlen, NULL, NULL);

			char processed_file_[MAX_PATH];
			sprintf(processed_file_, "%s/ProcessedAudioData.pcm", result);
			_recordProcesedFile = fopen(processed_file_, "wb");
		}

		if (_recordProcesedFile)
		{
			fwrite(_audioFrame.data_, 2, _audioFrame.samples_per_channel_, _recordProcesedFile);
			fflush(_recordProcesedFile);
		}
	}

#endif
    if (swap_stereo_channels_ && stereo_codec_)
      // Only bother swapping if we're using a stereo codec.
      AudioFrameOperations::SwapStereoChannels(&_audioFrame);

    // --- Annoying typing detection (utilizes the APM/VAD decision)
#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
    TypingDetection(keyPressed);
#endif

    // --- Mute during DTMF tone if direct feedback is enabled
    if (_remainingMuteMicTimeMs > 0)
    {
        AudioFrameOperations::Mute(_audioFrame);
        _remainingMuteMicTimeMs -= 10;
        if (_remainingMuteMicTimeMs < 0)
        {
            _remainingMuteMicTimeMs = 0;
        }
    }

    // --- Mute signal
    if (_mute)
    {
        AudioFrameOperations::Mute(_audioFrame);
    }

    // --- Mix with file (does not affect the mixing frequency)
    if (_filePlaying)
    {
        MixOrReplaceAudioWithFile(_audioFrame.sample_rate_hz_);
    }

    // --- Record to file
    
    
    
    
//#if RECORD_AUDIO_TEST
    
    // --- Record to file
    bool file_recording = false;
    {
        CriticalSectionScoped cs(&_critSect);
        file_recording =  _fileRecording;
    }
    if (file_recording)
    {
        RecordAudioToFile(_audioFrame2Up.sample_rate_hz_);
        _writecount++;
    }
    
    //added by zenggq 20150118
    bool file_Callrecording = false;
    {
        CriticalSectionScoped cs(&_critSect);
        file_Callrecording =  _fileCallRecording;
    }
    if (file_Callrecording)
    {
        RecordAudioToFileCall(_audioFrame.sample_rate_hz_);
    }
    //end for added zenggq
//#endif
    
    
//    bool file_recording = false;
//    {
//        CriticalSectionScoped cs(&_critSect);
//        file_recording =  _fileRecording;
//    }
//    if (file_recording)
//    {
//        RecordAudioToFile(_audioFrame.sample_rate_hz_);
//    }

    {
      CriticalSectionScoped cs(&_callbackCritSect);
      if (external_postproc_ptr_) {
        external_postproc_ptr_->Process(-1, kRecordingAllChannelsMixed,
                                        _audioFrame.data_,
                                        _audioFrame.samples_per_channel_,
                                        _audioFrame.sample_rate_hz_,
                                        _audioFrame.num_channels_ == 2);
      }
    }

    // --- Measure audio level of speech after all processing.
    _audioLevel.ComputeLevel(_audioFrame);
    return 0;
}

int32_t
TransmitMixer::DemuxAndMix()
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::DemuxAndMix()");

    for (ChannelManager::Iterator it(_channelManagerPtr); it.IsValid();
         it.Increment())
    {
        Channel* channelPtr = it.GetChannel();
        if (channelPtr->Sending())
        {
            // Demultiplex makes a copy of its input.
            //channelPtr->Demultiplex(_audioFrame);
			channelPtr->Demultiplex(_audioFrame,_audioFrame2Up);
            channelPtr->PrepareEncodeAndSend(_audioFrame.sample_rate_hz_);
        }
    }
    return 0;
}

void TransmitMixer::DemuxAndMix(const int voe_channels[],
                                int number_of_voe_channels) {
  for (int i = 0; i < number_of_voe_channels; ++i) {
    voe::ChannelOwner ch = _channelManagerPtr->GetChannel(voe_channels[i]);
    voe::Channel* channel_ptr = ch.channel();
    if (channel_ptr) {
      if (channel_ptr->Sending()) {
        // Demultiplex makes a copy of its input.
        channel_ptr->Demultiplex(_audioFrame, _audioFrame2Up);
        channel_ptr->PrepareEncodeAndSend(_audioFrame.sample_rate_hz_);
      }
    }
  }
}

int32_t
TransmitMixer::EncodeAndSend()
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::EncodeAndSend()");

    for (ChannelManager::Iterator it(_channelManagerPtr); it.IsValid();
         it.Increment())
    {
        Channel* channelPtr = it.GetChannel();
        if (channelPtr->Sending())
        {
            channelPtr->EncodeAndSend();
        }
    }
    return 0;
}

void TransmitMixer::EncodeAndSend(const int voe_channels[],
                                  int number_of_voe_channels) {
  for (int i = 0; i < number_of_voe_channels; ++i) {
    voe::ChannelOwner ch = _channelManagerPtr->GetChannel(voe_channels[i]);
    voe::Channel* channel_ptr = ch.channel();
    if (channel_ptr && channel_ptr->Sending())
      channel_ptr->EncodeAndSend();
  }
}

uint32_t TransmitMixer::CaptureLevel() const
{
    return _captureLevel;
}

void
TransmitMixer::UpdateMuteMicrophoneTime(uint32_t lengthMs)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
               "TransmitMixer::UpdateMuteMicrophoneTime(lengthMs=%d)",
               lengthMs);
    _remainingMuteMicTimeMs = lengthMs;
}

int32_t
TransmitMixer::StopSend()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
               "TransmitMixer::StopSend()");
    _audioLevel.Clear();
    return 0;
}

int TransmitMixer::StartPlayingFileAsMicrophone(const char* fileName,
                                                bool loop,
                                                FileFormats format,
                                                int startPosition,
                                                float volumeScaling,
                                                int stopPosition,
                                                const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::StartPlayingFileAsMicrophone("
                 "fileNameUTF8[]=%s,loop=%d, format=%d, volumeScaling=%5.3f,"
                 " startPosition=%d, stopPosition=%d)", fileName, loop,
                 format, volumeScaling, startPosition, stopPosition);

    if (_filePlaying)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceWarning,
            "StartPlayingFileAsMicrophone() is already playing");
        return 0;
    }

    CriticalSectionScoped cs(&_critSect);

    // Destroy the old instance
    if (_filePlayerPtr)
    {
        _filePlayerPtr->RegisterModuleFileCallback(NULL);
        FilePlayer::DestroyFilePlayer(_filePlayerPtr);
        _filePlayerPtr = NULL;
    }

    // Dynamically create the instance
    _filePlayerPtr
        = FilePlayer::CreateFilePlayer(_filePlayerId,
                                       (const FileFormats) format);

    if (_filePlayerPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartPlayingFileAsMicrophone() filePlayer format isnot correct");
        return -1;
    }

    const uint32_t notificationTime(0);

    if (_filePlayerPtr->StartPlayingFile(
        fileName,
        loop,
        startPosition,
        volumeScaling,
        notificationTime,
        stopPosition,
        (const CodecInst*) codecInst) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartPlayingFile() failed to start file playout");
        _filePlayerPtr->StopPlayingFile();
        FilePlayer::DestroyFilePlayer(_filePlayerPtr);
        _filePlayerPtr = NULL;
        return -1;
    }

    _filePlayerPtr->RegisterModuleFileCallback(this);
    _filePlaying = true;

    return 0;
}

int TransmitMixer::StartPlayingFileAsMicrophone(InStream* stream,
                                                FileFormats format,
                                                int startPosition,
                                                float volumeScaling,
                                                int stopPosition,
                                                const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "TransmitMixer::StartPlayingFileAsMicrophone(format=%d,"
                 " volumeScaling=%5.3f, startPosition=%d, stopPosition=%d)",
                 format, volumeScaling, startPosition, stopPosition);

    if (stream == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartPlayingFileAsMicrophone() NULL as input stream");
        return -1;
    }

    if (_filePlaying)
    {
        _engineStatisticsPtr->SetLastError(
            VE_ALREADY_PLAYING, kTraceWarning,
            "StartPlayingFileAsMicrophone() is already playing");
        return 0;
    }

    CriticalSectionScoped cs(&_critSect);

    // Destroy the old instance
    if (_filePlayerPtr)
    {
        _filePlayerPtr->RegisterModuleFileCallback(NULL);
        FilePlayer::DestroyFilePlayer(_filePlayerPtr);
        _filePlayerPtr = NULL;
    }

    // Dynamically create the instance
    _filePlayerPtr
        = FilePlayer::CreateFilePlayer(_filePlayerId,
                                       (const FileFormats) format);

    if (_filePlayerPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceWarning,
            "StartPlayingFileAsMicrophone() filePlayer format isnot correct");
        return -1;
    }

    const uint32_t notificationTime(0);

    if (_filePlayerPtr->StartPlayingFile(
        (InStream&) *stream,
        startPosition,
        volumeScaling,
        notificationTime,
        stopPosition,
        (const CodecInst*) codecInst) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartPlayingFile() failed to start file playout");
        _filePlayerPtr->StopPlayingFile();
        FilePlayer::DestroyFilePlayer(_filePlayerPtr);
        _filePlayerPtr = NULL;
        return -1;
    }
    _filePlayerPtr->RegisterModuleFileCallback(this);
    _filePlaying = true;

    return 0;
}

int TransmitMixer::StopPlayingFileAsMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "TransmitMixer::StopPlayingFileAsMicrophone()");

    if (!_filePlaying)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_OPERATION, kTraceWarning,
            "StopPlayingFileAsMicrophone() isnot playing");
        return 0;
    }

    CriticalSectionScoped cs(&_critSect);

    if (_filePlayerPtr->StopPlayingFile() != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_CANNOT_STOP_PLAYOUT, kTraceError,
            "StopPlayingFile() couldnot stop playing file");
        return -1;
    }

    _filePlayerPtr->RegisterModuleFileCallback(NULL);
    FilePlayer::DestroyFilePlayer(_filePlayerPtr);
    _filePlayerPtr = NULL;
    _filePlaying = false;

    return 0;
}

int TransmitMixer::IsPlayingFileAsMicrophone() const
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::IsPlayingFileAsMicrophone()");
    return _filePlaying;
}

int TransmitMixer::StartRecordingMicrophone(const char* fileName,
                                            const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::StartRecordingMicrophone(fileName=%s)",
                 fileName);

    CriticalSectionScoped cs(&_critSect);

    if (_fileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "StartRecordingMicrophone() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0); // Not supported in VoE
    CodecInst dummyCodec = { 100, "L16", 16000, 320, 1, 320000 };

    if (codecInst != NULL &&
      (codecInst->channels < 0 || codecInst->channels > 2))
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingMicrophone() invalid compression");
        return (-1);
    }
    if (codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst = &dummyCodec;
    } else if ((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    } else
    {
        format = kFileFormatCompressedFile;
    }

    // Destroy the old instance
    if (_fileRecorderPtr)
    {
        _fileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_fileRecorderPtr);
        _fileRecorderPtr = NULL;
    }

    _fileRecorderPtr =
        FileRecorder::CreateFileRecorder(_fileRecorderId,
                                         (const FileFormats) format);
    if (_fileRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingMicrophone() fileRecorder format isnot correct");
        return -1;
    }

    if (_fileRecorderPtr->StartRecordingAudioFile(
        fileName,
        (const CodecInst&) *codecInst,
        notificationTime) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartRecordingAudioFile() failed to start file recording");
        _fileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_fileRecorderPtr);
        _fileRecorderPtr = NULL;
        return -1;
    }
    _fileRecorderPtr->RegisterModuleFileCallback(this);
    _fileRecording = true;

    return 0;
}

int TransmitMixer::StartRecordingMicrophone(OutStream* stream,
                                            const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
               "TransmitMixer::StartRecordingMicrophone()");

    CriticalSectionScoped cs(&_critSect);

    if (_fileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                   "StartRecordingMicrophone() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0); // Not supported in VoE
    CodecInst dummyCodec = { 100, "L16", 16000, 320, 1, 320000 };

    if (codecInst != NULL && codecInst->channels != 1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingMicrophone() invalid compression");
        return (-1);
    }
    if (codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst = &dummyCodec;
    } else if ((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    } else
    {
        format = kFileFormatCompressedFile;
    }

    // Destroy the old instance
    if (_fileRecorderPtr)
    {
        _fileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_fileRecorderPtr);
        _fileRecorderPtr = NULL;
    }

    _fileRecorderPtr =
        FileRecorder::CreateFileRecorder(_fileRecorderId,
                                         (const FileFormats) format);
    if (_fileRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingMicrophone() fileRecorder format isnot correct");
        return -1;
    }

    if (_fileRecorderPtr->StartRecordingAudioFile(*stream,
                                                  *codecInst,
                                                  notificationTime) != 0)
    {
    _engineStatisticsPtr->SetLastError(VE_BAD_FILE, kTraceError,
      "StartRecordingAudioFile() failed to start file recording");
    _fileRecorderPtr->StopRecording();
    FileRecorder::DestroyFileRecorder(_fileRecorderPtr);
    _fileRecorderPtr = NULL;
    return -1;
    }

    _fileRecorderPtr->RegisterModuleFileCallback(this);
    _fileRecording = true;

    return 0;
}


int TransmitMixer::StopRecordingMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::StopRecordingMicrophone()");

    CriticalSectionScoped cs(&_critSect);

    if (!_fileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                   "StopRecordingMicrophone() isnot recording");
        return 0;
    }

    if (_fileRecorderPtr->StopRecording() != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_STOP_RECORDING_FAILED, kTraceError,
            "StopRecording(), could not stop recording");
        return -1;
    }
    _fileRecorderPtr->RegisterModuleFileCallback(NULL);
    FileRecorder::DestroyFileRecorder(_fileRecorderPtr);
    _fileRecorderPtr = NULL;
    _fileRecording = false;

    return 0;
}

int TransmitMixer::StartRecordingCall(const char* fileName,
                                      const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::StartRecordingCall(fileName=%s)", fileName);

    if (_fileCallRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "StartRecordingCall() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0); // Not supported in VoE
    CodecInst dummyCodec = { 100, "L16", 16000, 320, 1, 320000 };

    if (codecInst != NULL && codecInst->channels != 1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingCall() invalid compression");
        return (-1);
    }
    if (codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst = &dummyCodec;
    } else if ((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    } else
    {
        format = kFileFormatCompressedFile;
    }

    CriticalSectionScoped cs(&_critSect);

    // Destroy the old instance
    if (_fileCallRecorderPtr)
    {
        _fileCallRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_fileCallRecorderPtr);
        _fileCallRecorderPtr = NULL;
    }

    _fileCallRecorderPtr
        = FileRecorder::CreateFileRecorder(_fileCallRecorderId,
                                           (const FileFormats) format);
    if (_fileCallRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingCall() fileRecorder format isnot correct");
        return -1;
    }

    if (_fileCallRecorderPtr->StartRecordingAudioFile(
        fileName,
        (const CodecInst&) *codecInst,
        notificationTime) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartRecordingAudioFile() failed to start file recording");
        _fileCallRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_fileCallRecorderPtr);
        _fileCallRecorderPtr = NULL;
        return -1;
    }
    _fileCallRecorderPtr->RegisterModuleFileCallback(this);
    _fileCallRecording = true;

    return 0;
}

int TransmitMixer::StartRecordingCall(OutStream* stream,
                                      const  CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::StartRecordingCall()");

    if (_fileCallRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "StartRecordingCall() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0); // Not supported in VoE
    CodecInst dummyCodec = { 100, "L16", 16000, 320, 1, 320000 };

    if (codecInst != NULL && codecInst->channels != 1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingCall() invalid compression");
        return (-1);
    }
    if (codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst = &dummyCodec;
    } else if ((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    } else
    {
        format = kFileFormatCompressedFile;
    }

    CriticalSectionScoped cs(&_critSect);

    // Destroy the old instance
    if (_fileCallRecorderPtr)
    {
        _fileCallRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_fileCallRecorderPtr);
        _fileCallRecorderPtr = NULL;
    }

    _fileCallRecorderPtr =
        FileRecorder::CreateFileRecorder(_fileCallRecorderId,
                                         (const FileFormats) format);
    if (_fileCallRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingCall() fileRecorder format isnot correct");
        return -1;
    }

    if (_fileCallRecorderPtr->StartRecordingAudioFile(*stream,
                                                      *codecInst,
                                                      notificationTime) != 0)
    {
    _engineStatisticsPtr->SetLastError(VE_BAD_FILE, kTraceError,
     "StartRecordingAudioFile() failed to start file recording");
    _fileCallRecorderPtr->StopRecording();
    FileRecorder::DestroyFileRecorder(_fileCallRecorderPtr);
    _fileCallRecorderPtr = NULL;
    return -1;
    }

    _fileCallRecorderPtr->RegisterModuleFileCallback(this);
    _fileCallRecording = true;

    return 0;
}

int TransmitMixer::StopRecordingCall()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::StopRecordingCall()");

    if (!_fileCallRecording)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId, -1),
                     "StopRecordingCall() file isnot recording");
        return -1;
    }

    CriticalSectionScoped cs(&_critSect);

    if (_fileCallRecorderPtr->StopRecording() != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_STOP_RECORDING_FAILED, kTraceError,
            "StopRecording(), could not stop recording");
        return -1;
    }

    _fileCallRecorderPtr->RegisterModuleFileCallback(NULL);
    FileRecorder::DestroyFileRecorder(_fileCallRecorderPtr);
    _fileCallRecorderPtr = NULL;
    _fileCallRecording = false;

    return 0;
}

void
TransmitMixer::SetMixWithMicStatus(bool mix)
{
    _mixFileWithMicrophone = mix;
}

int TransmitMixer::RegisterExternalMediaProcessing(
    VoEMediaProcess* object,
    ProcessingTypes type) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
               "TransmitMixer::RegisterExternalMediaProcessing()");

  CriticalSectionScoped cs(&_callbackCritSect);
  if (!object) {
    return -1;
  }

  // Store the callback object according to the processing type.
  if (type == kRecordingAllChannelsMixed) {
    external_postproc_ptr_ = object;
  } else if (type == kRecordingPreprocessing) {
    external_preproc_ptr_ = object;
  } else {
    return -1;
  }
  return 0;
}

int TransmitMixer::DeRegisterExternalMediaProcessing(ProcessingTypes type) {
  WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
               "TransmitMixer::DeRegisterExternalMediaProcessing()");

  CriticalSectionScoped cs(&_callbackCritSect);
  if (type == kRecordingAllChannelsMixed) {
    external_postproc_ptr_ = NULL;
  } else if (type == kRecordingPreprocessing) {
    external_preproc_ptr_ = NULL;
  } else {
    return -1;
  }
  return 0;
}

int
TransmitMixer::SetMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "TransmitMixer::SetMute(enable=%d)", enable);
    _mute = enable;
    return 0;
}

bool
TransmitMixer::Mute() const
{
    return _mute;
}

int8_t TransmitMixer::AudioLevel() const
{
    // Speech + file level [0,9]
    return _audioLevel.Level();
}

int16_t TransmitMixer::AudioLevelFullRange() const
{
    // Speech + file level [0,32767]
    return _audioLevel.LevelFullRange();
}

bool TransmitMixer::IsRecordingCall()
{
    return _fileCallRecording;
}

bool TransmitMixer::IsRecordingMic()
{
    CriticalSectionScoped cs(&_critSect);
    return _fileRecording;
}

#ifdef WIN32_MIC
bool TransmitMixer::silence(const short* data, int sample_length, float thres, float probility)
{
	int real_thres = 0;
	float f = powf(10, thres / 20.0f) * powf(2, 16);
	real_thres = f + 0.5f;
	int cnt = 0;
	for (int i = 0; i < sample_length; ++i)
	{
		if (data[i] > real_thres || data[i] < -real_thres)
			cnt++;
	}
	if (cnt / (float)sample_length > probility)
		return false;
	else
		return true;
}
#endif

void TransmitMixer::GenerateAudioFrame(const int16_t* audio,
                                       int samples_per_channel,
                                       int num_channels,
                                       int sample_rate_hz) {
  int codec_rate;
  int num_codec_channels;
  GetSendCodecInfo(&codec_rate, &num_codec_channels);
  // TODO(ajm): This currently restricts the sample rate to 32 kHz.
  // See: https://code.google.com/p/webrtc/issues/detail?id=3146
  // When 48 kHz is supported natively by AudioProcessing, this will have
  // to be changed to handle 44.1 kHz.
  int max_sample_rate_hz = kAudioProcMaxNativeSampleRateHz;
  if (audioproc_->echo_control_mobile()->is_enabled()) {
    // AECM only supports 8 and 16 kHz.
    max_sample_rate_hz = 16000;
  }
  codec_rate = std::min(codec_rate, max_sample_rate_hz);
  stereo_codec_ = num_codec_channels == 2;

  if (!mono_buffer_.get()) {
    // Temporary space for DownConvertToCodecFormat.
    mono_buffer_.reset(new int16_t[kMaxMonoDataSizeSamples]);
  }
  DownConvertToCodecFormat(audio,
                           samples_per_channel,
                           num_channels,
                           sample_rate_hz,
                           num_codec_channels,
                           codec_rate,
                           mono_buffer_.get(),
                           &resampler_,
                           &_audioFrame);
#ifdef WIN32_MIC
  //save 1s data; 16k,1channel,10ms = 320bytes; 
	static int currentDataLen = 0;

	if (currentDataLen >= 32000) 
		currentDataLen = 0;

	memcpy((char *)_data1S+currentDataLen, (char *)_audioFrame.data_, 320);
	currentDataLen += 320;
#endif
}
/*zeng guo qing's algorithm*/
#if 0
bool TransmitMixer::IsSilence()
{
	return silence(_data1S, 16000, (float)-65.0, (float)0.1);
}
#endif

#ifdef WIN32_MIC
bool TransmitMixer::IsSilence()
{
	int sum = 0;
	for (int i = 0; i < 16000; i++) {
		if (_data1S[i] >= 0)
			sum += _data1S[i];
		else
			sum -= _data1S[i];
	}

	float db = 20 * log10(sum/16000.0/32768);

	//WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId, -1), "db = %f", db);
	int db_int = (int)(db - 0.5);
	return (db_int<=-67 && db_int>=-69);
}
#endif

int32_t TransmitMixer::RecordAudioToFile(
    uint32_t mixingFrequency)
{
    CriticalSectionScoped cs(&_critSect);
    if (_fileRecorderPtr == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "TransmitMixer::RecordAudioToFile() filerecorder doesnot"
                     "exist");
        return -1;
    }

    if (_fileRecorderPtr->RecordAudioToFile(_audioFrame2Up) != 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "TransmitMixer::RecordAudioToFile() file recording"
                     "failed");
        return -1;
    }

    return 0;
}

int32_t TransmitMixer::MixOrReplaceAudioWithFile(
    int mixingFrequency)
{
    scoped_ptr<int16_t[]> fileBuffer(new int16_t[640]);

    int fileSamples(0);
    {
        CriticalSectionScoped cs(&_critSect);
        if (_filePlayerPtr == NULL)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice,
                         VoEId(_instanceId, -1),
                         "TransmitMixer::MixOrReplaceAudioWithFile()"
                         "fileplayer doesnot exist");
            return -1;
        }

        if (_filePlayerPtr->Get10msAudioFromFile(fileBuffer.get(),
                                                 fileSamples,
                                                 mixingFrequency) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                         "TransmitMixer::MixOrReplaceAudioWithFile() file"
                         " mixing failed");
            return -1;
        }
    }

    assert(_audioFrame.samples_per_channel_ == fileSamples);

    if (_mixFileWithMicrophone)
    {
        // Currently file stream is always mono.
        // TODO(xians): Change the code when FilePlayer supports real stereo.
        MixWithSat(_audioFrame.data_,
                   _audioFrame.num_channels_,
                   fileBuffer.get(),
                   1,
                   fileSamples);
    } else
    {
        // Replace ACM audio with file.
        // Currently file stream is always mono.
        // TODO(xians): Change the code when FilePlayer supports real stereo.
        _audioFrame.UpdateFrame(-1,
                                0xFFFFFFFF,
                                fileBuffer.get(),
                                fileSamples,
                                mixingFrequency,
                                AudioFrame::kNormalSpeech,
                                AudioFrame::kVadUnknown,
                                1);
    }
    return 0;
}

void TransmitMixer::ProcessAudio(int delay_ms, int clock_drift,
                                 int current_mic_level, bool key_pressed) {
  if (audioproc_->set_stream_delay_ms(delay_ms) != 0) {
    // A redundant warning is reported in AudioDevice, which we've throttled
    // to avoid flooding the logs. Relegate this one to LS_VERBOSE to avoid
    // repeating the problem here.
    LOG_FERR1(LS_VERBOSE, set_stream_delay_ms, delay_ms);
  }

  GainControl* agc = audioproc_->gain_control();
  if (agc->set_stream_analog_level(current_mic_level) != 0) {
    LOG_FERR1(LS_ERROR, set_stream_analog_level, current_mic_level);
    assert(false);
  }

  EchoCancellation* aec = audioproc_->echo_cancellation();
  if (aec->is_drift_compensation_enabled()) {
    aec->set_stream_drift_samples(clock_drift);
  }

  audioproc_->set_stream_key_pressed(key_pressed);

  int err = audioproc_->ProcessStream(&_audioFrame);
  if (err != 0) {
    LOG(LS_ERROR) << "ProcessStream() error: " << err;
    assert(false);
  }

  // Store new capture level. Only updated when analog AGC is enabled.
  _captureLevel = agc->stream_analog_level();

  CriticalSectionScoped cs(&_critSect);
  // Triggers a callback in OnPeriodicProcess().
  _saturationWarning |= agc->stream_is_saturated();
}

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
void TransmitMixer::TypingDetection(bool keyPressed)
{
  // We let the VAD determine if we're using this feature or not.
  if (_audioFrame.vad_activity_ == AudioFrame::kVadUnknown) {
    return;
  }

  bool vadActive = _audioFrame.vad_activity_ == AudioFrame::kVadActive;
  if (_typingDetection.Process(keyPressed, vadActive)) {
    _typingNoiseWarningPending = true;
    _typingNoiseDetected = true;
  } else {
    // If there is already a warning pending, do not change the state.
    // Otherwise set a warning pending if last callback was for noise detected.
    if (!_typingNoiseWarningPending && _typingNoiseDetected) {
      _typingNoiseWarningPending = true;
      _typingNoiseDetected = false;
    }
  }
}
#endif

int TransmitMixer::GetMixingFrequency()
{
    assert(_audioFrame.sample_rate_hz_ != 0);
    return _audioFrame.sample_rate_hz_;
}

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
int TransmitMixer::TimeSinceLastTyping(int &seconds)
{
    // We check in VoEAudioProcessingImpl that this is only called when
    // typing detection is active.
    seconds = _typingDetection.TimeSinceLastDetectionInSeconds();
    return 0;
}
#endif

#ifdef WEBRTC_VOICE_ENGINE_TYPING_DETECTION
int TransmitMixer::SetTypingDetectionParameters(int timeWindow,
                                                int costPerTyping,
                                                int reportingThreshold,
                                                int penaltyDecay,
                                                int typeEventDelay)
{
    _typingDetection.SetParameters(timeWindow,
                                   costPerTyping,
                                   reportingThreshold,
                                   penaltyDecay,
                                   typeEventDelay,
                                   0);
    return 0;
}
#endif

void TransmitMixer::EnableStereoChannelSwapping(bool enable) {
  swap_stereo_channels_ = enable;
}

bool TransmitMixer::IsStereoChannelSwappingEnabled() {
  return swap_stereo_channels_;
}

//    Sean add begin 20131119 noise suppression

WebRtc_Word32
	TransmitMixer::GenerateAudioFrameNoiseSuppression(const WebRtc_Word16 audioSamples[],
	const WebRtc_UWord32 nSamples,
	const WebRtc_UWord8 nChannels,
	const WebRtc_UWord32 samplesPerSec,
	const int mixingFrequency)
{
	WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId, -1),
		"TransmitMixer::GenerateAudioFrame(nSamples=%u,"
		"samplesPerSec=%u, mixingFrequency=%u)",
		nSamples, samplesPerSec, mixingFrequency);

	//	ResamplerType resampType = (nChannels == 1) ? kResamplerSynchronous : kResamplerSynchronousStereo;
	//
	//
	//    if (_audioResamplerNoiseSuppression.ResetIfNeeded(samplesPerSec,
	//                                      mixingFrequency,
	//                                      resampType) != 0)
	//    {
	//        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId, -1),
	//                     "TransmitMixer::GenerateAudioFrame() unable to resample");
	//        return -1;
	//    }

	if (_audioResamplerNoiseSuppression.SetModeForNoiseSupprion() != 0)
	{
		WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId, -1),
			"TransmitMixer::GenerateAudioFrame() unable to resample");
		return -1;
	}
	if (_audioResamplerNoiseSuppression.Push(
		(WebRtc_Word16*) audioSamples,
		nSamples * nChannels,
		_audioFrameNoiseSuppression.data_,
		AudioFrame::kMaxDataSizeSamples,
		(int&) _audioFrameNoiseSuppression.samples_per_channel_) == -1)
	{
		WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId, -1),
			"TransmitMixer::GenerateAudioFrame() resampling failed");
		return -1;
	}

	_audioFrameNoiseSuppression.samples_per_channel_ /= nChannels;
	_audioFrameNoiseSuppression.id_ = _instanceId;
	_audioFrameNoiseSuppression.timestamp_ = -1;
	_audioFrameNoiseSuppression.sample_rate_hz_ = mixingFrequency;
	_audioFrameNoiseSuppression.speech_type_ = AudioFrame::kNormalSpeech;
	_audioFrameNoiseSuppression.vad_activity_ = AudioFrame::kVadUnknown;
	_audioFrameNoiseSuppression.num_channels_ = nChannels;

	return 0;
}


WebRtc_Word32 TransmitMixer::APMProcessStreamNoiseSuppression(
	const WebRtc_UWord16 totalDelayMS,
	const WebRtc_Word32 clockDrift,
	const WebRtc_UWord16 currentMicLevel)
{
	WebRtc_UWord16 captureLevel(currentMicLevel);

	// Check if the number of input channels has changed. Retain the number
	// of output channels.
	//    sean add begin transition solution for media_init_audio failed 20140110 begin
	if (!audioproc_) {
		return -1;
	}
	//    sean add begin transition solution for media_init_audio failed 20140110 end

	if (_audioFrameNoiseSuppression.num_channels_ !=
		audioproc_->num_input_channels())
	{
		if (audioproc_->set_num_channels(
			_audioFrame.num_channels_,
			audioproc_->num_output_channels()))
		{
			WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
				"AudioProcessing::set_num_channels(%d, %d) => error",
				_audioFrame.num_channels_,
				audioproc_->num_output_channels());
		}
	}

	// If the frequency has changed we need to change APM settings
	// Sending side is "master"
	if (audioproc_->sample_rate_hz() !=
		_audioFrame.sample_rate_hz_)
	{
		if (audioproc_->set_sample_rate_hz(
			_audioFrame.sample_rate_hz_))
		{
			WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
				"AudioProcessing::set_sample_rate_hz(%u) => error",
				_audioFrame.sample_rate_hz_);
		}
	}

	if (audioproc_->set_stream_delay_ms(totalDelayMS) == -1)
	{
		WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
			"AudioProcessing::set_stream_delay_ms(%u) => error",
			totalDelayMS);
	}
	if (audioproc_->gain_control()->set_stream_analog_level(
		captureLevel) == -1)
	{
		WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
			"AudioProcessing::set_stream_analog_level(%u) => error",
			captureLevel);
	}
	if (audioproc_->echo_cancellation()->
		is_drift_compensation_enabled())
	{
		audioproc_->echo_cancellation()->
			set_stream_drift_samples(clockDrift);

		/*if (audioproc_->echo_cancellation()->
			set_stream_drift_samples(clockDrift) == -1)
		{
			WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
				"AudioProcessing::set_stream_drift_samples(%u) => error",
				clockDrift);
		}*/
	}
	if (audioproc_->ProcessStreamNoiseSuppression(&_audioFrameNoiseSuppression) == -1)
	{
		WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
			"AudioProcessing::ProcessStream() => error");
	}
	captureLevel =
		audioproc_->gain_control()->stream_analog_level();

	// Store new capture level (only updated when analog AGC is enabled)
	_captureLevel = captureLevel;

	// Log notifications
	if (audioproc_->gain_control()->stream_is_saturated())
	{
		if (_saturationWarning == 1)
		{
			WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
				"TransmitMixer::APMProcessStreamNoiseSuppression() pending "
				"saturation warning exists");
		}
		_saturationWarning = 1; // triggers callback from moduleprocess thread
		WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
			"TransmitMixer::APMProcessStreamNoiseSuppression() VE_SATURATION_WARNING "
			"message has been posted for callback");
	}

	return 0;
}

//    Sean add end 20131119 noise suppression

}  // namespace voe
}  // namespace webrtc
