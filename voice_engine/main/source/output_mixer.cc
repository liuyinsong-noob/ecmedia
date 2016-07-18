/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "output_mixer.h"

#include "audio_processing.h"
#include "audio_frame_operations.h"
#include "critical_section_wrapper.h"
#include "file_wrapper.h"
#include "trace.h"
#include "voe_external_media.h"
#include "statistics.h"
#include "utility.h"
char *filenameRender_path=NULL;

namespace cloopenwebrtc {
namespace voe {

#define ENERGY_MIN_8K    (2.0*1e8)
#define ENERGY_MAX_SILENCE_8K (1.0*1e7)

int Writebuf(void* buf_ring, int16* pIndata, int samples);
int Initbuf(void* buf_ring, int samplerate);
int CheckEnergy(void* buf_ring);
float GetEnergy(void* buf_ring);
int Iskeyframe(int16_t* pIndata, int samples, void* buf_ring);
int sign( int16_t& a );


int Writebuf(void* buf_ring, int16* pIndata, int samples)
{
    ringbuf* ptr = (ringbuf*)buf_ring;
    if( ptr )
    {
        if( ptr->iswarp )
        {
            float energy = 0.0f;
            for( int i = 0; i < samples; ++i )
            {
                energy += pIndata[i]*pIndata[i];
            }
            *(ptr->buf + ptr->write_pos) = energy;
            ptr->write_pos += 1;
            if( ptr->write_pos >= ptr->max_sample )
                ptr->write_pos = 0;
        }
        else
        {
            float energy = 0.0f;
            for( int i = 0; i < samples; ++i )
            {
                energy += pIndata[i]*pIndata[i];
            }
            *(ptr->buf + ptr->write_pos) = energy;
            
            ptr->write_pos += 1;
            if( ptr->write_pos >= ptr->max_sample )
            {
                ptr->write_pos = 0;
                ptr->iswarp    = 1;
            }
        }
    }
    
    return 1;
}

int Initbuf(void* buf_ring, int samplerate)
{
    ringbuf* ptr = (ringbuf*)buf_ring;
    
    if( ptr )
    {
        memset(ptr->buf, 0, 20*sizeof(float));
        ptr->iswarp     = 0;
        ptr->write_pos  = 0;
        ptr->max_sample = 15;
    }
    
    return 1;
}

int CheckEnergy(void* buf_ring)
{
    ringbuf* ptr = (ringbuf*)buf_ring;
    if( ptr->iswarp )
    {
        if( ptr->write_pos + 10 <= ptr->max_sample )
        {
            for( int i = 0; i < 10; ++i )
            {
                if( ptr->buf[ptr->write_pos+i] >= ENERGY_MAX_SILENCE_8K )
                    return 0;
            }
            return 1;
        }
        else
        {
            int count = 0;
            for( int i = ptr->write_pos; i < ptr->max_sample; ++i )
            {
                if( ptr->buf[ptr->write_pos+count] >= ENERGY_MAX_SILENCE_8K )
                    return 0;
                count++;
            }
            for( int i = count; i < 10; ++i )
            {
                if( ptr->buf[i-count] >= ENERGY_MAX_SILENCE_8K )
                    return 0;
            }
            return 1;
        }
    }
    else
    {
        return 0;
    }
}

    
float GetEnergy(void* buf_ring)
{
    float energy = 0.0f;
    ringbuf* ptr = (ringbuf*)buf_ring;
    
    
    if( ptr->iswarp )
    {
        int max = ptr->max_sample;
        for( int i = 0; i < max; ++i )
            energy += ptr->buf[i];
        
        if( ptr->write_pos > 0 )
            energy -= ptr->buf[ptr->write_pos - 1];
        else
            energy -= ptr->buf[ptr->max_sample - 1];
    }
    else
    {
        int max = ptr->write_pos;
        for( int i = 0; i < max; ++i )
            energy += ptr->buf[i]*ptr->buf[i];
        
        if( max > 0 )
            energy -= ptr->buf[max - 1];
        
    }
    
    
    return energy;
}
    
    int sign( int16_t& a )
    {
        if( a >= 0 )
            return 1;
        else
            return -1;
    }
    
    int Iskeyframe(int16_t* pIndata, int samples, void* buf_ring)
    {
#if 1
        return 0;
#endif
        ringbuf* ptr_buf = (ringbuf*)buf_ring;
        
        int zeros    = 0;
        for( int i = 1; i < samples; ++i )
        {
            zeros += abs(sign(pIndata[i]) - sign(pIndata[i-1]));
        }
        
        if( CheckEnergy(buf_ring) != 1 )
        {
            Writebuf(buf_ring, pIndata, samples);
            return 0;
        }
        
        if( zeros < 20 || zeros > 100 )
        {
            int min = 32767;
            int max = -32768;
            float energy = 0.f;
            for( int i = 0; i < samples; ++i )
            {
                if( min > pIndata[i] )
                    min = pIndata[i];
                if( max < pIndata[i] )
                    max = pIndata[i];
                
                energy += pIndata[i]*pIndata[i];
            }
            
            if( max > 8000 || (min < -8000 && energy >= ENERGY_MIN_8K && ptr_buf->iswarp && 3*GetEnergy(buf_ring) < energy) )
            {
                Writebuf(buf_ring, pIndata, samples);
                return 1;
            }
        }
        
        Writebuf(buf_ring, pIndata, samples);
        
        return 0;
    }
    
void
OutputMixer::NewMixedAudio(int32_t id,
                           const AudioFrame& generalAudioFrame,
                           const AudioFrame** uniqueAudioFrames,
                           uint32_t size)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::NewMixedAudio(id=%d, size=%u)", id, size);

    _audioFrame.CopyFrom(generalAudioFrame);
    _audioFrame.id_ = id;
}

void OutputMixer::MixedParticipants(
    int32_t id,
    const ParticipantStatistics* participantStatistics,
    uint32_t size)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::MixedParticipants(id=%d, size=%u)", id, size);
}

void OutputMixer::VADPositiveParticipants(int32_t id,
    const ParticipantStatistics* participantStatistics, uint32_t size)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::VADPositiveParticipants(id=%d, size=%u)",
                 id, size);
}

void OutputMixer::MixedAudioLevel(int32_t id, uint32_t level)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::MixedAudioLevel(id=%d, level=%u)", id, level);
}

void OutputMixer::PlayNotification(int32_t id, uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::PlayNotification(id=%d, durationMs=%d)",
                 id, durationMs);
    // Not implement yet
}

void OutputMixer::RecordNotification(int32_t id,
                                     uint32_t durationMs)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::RecordNotification(id=%d, durationMs=%d)",
                 id, durationMs);

    // Not implement yet
}

void OutputMixer::PlayFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::PlayFileEnded(id=%d)", id);

    // not needed
}

void OutputMixer::RecordFileEnded(int32_t id)
{
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::RecordFileEnded(id=%d)", id);
    assert(id == _instanceId);

    CriticalSectionScoped cs(&_fileCritSect);
    _outputFileRecording = false;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::RecordFileEnded() =>"
                 "output file recorder module is shutdown");
}

int32_t
OutputMixer::Create(OutputMixer*& mixer, uint32_t instanceId)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, instanceId,
                 "OutputMixer::Create(instanceId=%d)", instanceId);
    mixer = new OutputMixer(instanceId);
    if (mixer == NULL)
    {
        WEBRTC_TRACE(kTraceMemory, kTraceVoice, instanceId,
                     "OutputMixer::Create() unable to allocate memory for"
                     "mixer");
        return -1;
    }
    return 0;
}

OutputMixer::OutputMixer(uint32_t instanceId) :
    _callbackCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _fileCritSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _mixerModule(*AudioConferenceMixer::Create(instanceId)),
    _audioLevel(),
    _dtmfGenerator(instanceId),
    _instanceId(instanceId),
    _externalMediaCallbackPtr(NULL),
    _externalMedia(false),
    _panLeft(1.0f),
    _panRight(1.0f),
    _mixingFrequencyHz(8000),
    _outputFileRecorderPtr(NULL),
    _outputFileRecording(false),
    _samplerate(0),
    _writecount(0),
    _fileindex(1)
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::OutputMixer() - ctor");

    if ((_mixerModule.RegisterMixedStreamCallback(*this) == -1) ||
        (_mixerModule.RegisterMixerStatusCallback(*this, 100) == -1))
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,-1),
                     "OutputMixer::OutputMixer() failed to register mixer"
                     "callbacks");
    }

    _dtmfGenerator.Init();
}

void
OutputMixer::Destroy(OutputMixer*& mixer)
{
    if (mixer)
    {
        delete mixer;
        mixer = NULL;
    }
}

OutputMixer::~OutputMixer()
{
    WEBRTC_TRACE(kTraceMemory, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::~OutputMixer() - dtor");
    if (_externalMedia)
    {
        DeRegisterExternalMediaProcessing();
    }
    {
        CriticalSectionScoped cs(&_fileCritSect);
        if (_outputFileRecorderPtr)
        {
            _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
            _outputFileRecorderPtr->StopRecording();
            FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
            _outputFileRecorderPtr = NULL;
        }
    }
    _mixerModule.UnRegisterMixerStatusCallback();
    _mixerModule.UnRegisterMixedStreamCallback();
    delete &_mixerModule;
    delete &_callbackCritSect;
    delete &_fileCritSect;
}

int32_t
OutputMixer::SetEngineInformation(voe::Statistics& engineStatistics)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::SetEngineInformation()");
    _engineStatisticsPtr = &engineStatistics;
    return 0;
}

int32_t
OutputMixer::SetAudioProcessingModule(AudioProcessing* audioProcessingModule)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::SetAudioProcessingModule("
                 "audioProcessingModule=0x%x)", audioProcessingModule);
    _audioProcessingModulePtr = audioProcessingModule;
    return 0;
}

int OutputMixer::RegisterExternalMediaProcessing(
    VoEMediaProcess& proccess_object)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
               "OutputMixer::RegisterExternalMediaProcessing()");

    CriticalSectionScoped cs(&_callbackCritSect);
    _externalMediaCallbackPtr = &proccess_object;
    _externalMedia = true;

    return 0;
}

int OutputMixer::DeRegisterExternalMediaProcessing()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::DeRegisterExternalMediaProcessing()");

    CriticalSectionScoped cs(&_callbackCritSect);
    _externalMedia = false;
    _externalMediaCallbackPtr = NULL;

    return 0;
}

int OutputMixer::PlayDtmfTone(uint8_t eventCode, int lengthMs,
                              int attenuationDb)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId, -1),
                 "OutputMixer::PlayDtmfTone()");
    if (_dtmfGenerator.AddTone(eventCode, lengthMs, attenuationDb) != 0)
    {
        _engineStatisticsPtr->SetLastError(VE_STILL_PLAYING_PREV_DTMF,
                                           kTraceError,
                                           "OutputMixer::PlayDtmfTone()");
        return -1;
    }
    return 0;
}

int32_t
OutputMixer::SetMixabilityStatus(MixerParticipant& participant,
                                 bool mixable)
{
    return _mixerModule.SetMixabilityStatus(participant, mixable);
}

int32_t
OutputMixer::SetAnonymousMixabilityStatus(MixerParticipant& participant,
                                          bool mixable)
{
    return _mixerModule.SetAnonymousMixabilityStatus(participant,mixable);
}

int32_t
OutputMixer::MixActiveChannels()
{
    return _mixerModule.Process();
}

int
OutputMixer::GetSpeechOutputLevel(uint32_t& level)
{
    int8_t currentLevel = _audioLevel.Level();
    level = static_cast<uint32_t> (currentLevel);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "GetSpeechOutputLevel() => level=%u", level);
    return 0;
}

int
OutputMixer::GetSpeechOutputLevelFullRange(uint32_t& level)
{
    int16_t currentLevel = _audioLevel.LevelFullRange();
    level = static_cast<uint32_t> (currentLevel);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "GetSpeechOutputLevelFullRange() => level=%u", level);
    return 0;
}

int
OutputMixer::SetOutputVolumePan(float left, float right)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::SetOutputVolumePan()");
    _panLeft = left;
    _panRight = right;
    return 0;
}

int
OutputMixer::GetOutputVolumePan(float& left, float& right)
{
    left = _panLeft;
    right = _panRight;
    WEBRTC_TRACE(kTraceStateInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "GetOutputVolumePan() => left=%2.1f, right=%2.1f",
                 left, right);
    return 0;
}

int OutputMixer::StartRecordingPlayout(const char* fileName,
                                       const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::StartRecordingPlayout(fileName=%s)", fileName);

    if (_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,-1),
                     "StartRecordingPlayout() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0);
    CodecInst dummyCodec={100,"L16",16000,320,1,320000};

    if ((codecInst != NULL) &&
      ((codecInst->channels < 1) || (codecInst->channels > 2)))
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingPlayout() invalid compression");
        return(-1);
    }
    if(codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst=&dummyCodec;
    }
    else if((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    }
    else
    {
        format = kFileFormatCompressedFile;
    }

    CriticalSectionScoped cs(&_fileCritSect);

    // Destroy the old instance
    if (_outputFileRecorderPtr)
    {
        _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
    }

    _outputFileRecorderPtr = FileRecorder::CreateFileRecorder(
        _instanceId,
        (const FileFormats)format);
    if (_outputFileRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingPlayout() fileRecorder format isnot correct");
        return -1;
    }

    if (_outputFileRecorderPtr->StartRecordingAudioFile(
        fileName,
        (const CodecInst&)*codecInst,
        notificationTime) != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_FILE, kTraceError,
            "StartRecordingAudioFile() failed to start file recording");
        _outputFileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(this);
    _outputFileRecording = true;

    return 0;
}

int OutputMixer::StartRecordingPlayout(OutStream* stream,
                                       const CodecInst* codecInst)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::StartRecordingPlayout()");

    if (_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,-1),
                     "StartRecordingPlayout() is already recording");
        return 0;
    }

    FileFormats format;
    const uint32_t notificationTime(0);
    CodecInst dummyCodec={100,"L16",16000,320,1,320000};

    if (codecInst != NULL && codecInst->channels != 1)
    {
        _engineStatisticsPtr->SetLastError(
            VE_BAD_ARGUMENT, kTraceError,
            "StartRecordingPlayout() invalid compression");
        return(-1);
    }
    if(codecInst == NULL)
    {
        format = kFileFormatPcm16kHzFile;
        codecInst=&dummyCodec;
    }
    else if((STR_CASE_CMP(codecInst->plname,"L16") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMU") == 0) ||
        (STR_CASE_CMP(codecInst->plname,"PCMA") == 0))
    {
        format = kFileFormatWavFile;
    }
    else
    {
        format = kFileFormatCompressedFile;
    }

    CriticalSectionScoped cs(&_fileCritSect);

    // Destroy the old instance
    if (_outputFileRecorderPtr)
    {
        _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
    }

    _outputFileRecorderPtr = FileRecorder::CreateFileRecorder(
        _instanceId,
        (const FileFormats)format);
    if (_outputFileRecorderPtr == NULL)
    {
        _engineStatisticsPtr->SetLastError(
            VE_INVALID_ARGUMENT, kTraceError,
            "StartRecordingPlayout() fileRecorder format isnot correct");
        return -1;
    }

    if (_outputFileRecorderPtr->StartRecordingAudioFile(*stream,
                                                        *codecInst,
                                                        notificationTime) != 0)
    {
       _engineStatisticsPtr->SetLastError(VE_BAD_FILE, kTraceError,
           "StartRecordingAudioFile() failed to start file recording");
        _outputFileRecorderPtr->StopRecording();
        FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
        _outputFileRecorderPtr = NULL;
        return -1;
    }

    _outputFileRecorderPtr->RegisterModuleFileCallback(this);
    _outputFileRecording = true;

    return 0;
}

int OutputMixer::StopRecordingPlayout()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::StopRecordingPlayout()");

    if (!_outputFileRecording)
    {
        WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId,-1),
                     "StopRecordingPlayout() file isnot recording");
        return -1;
    }

    CriticalSectionScoped cs(&_fileCritSect);

    if (_outputFileRecorderPtr->StopRecording() != 0)
    {
        _engineStatisticsPtr->SetLastError(
            VE_STOP_RECORDING_FAILED, kTraceError,
            "StopRecording(), could not stop recording");
        return -1;
    }
    _outputFileRecorderPtr->RegisterModuleFileCallback(NULL);
    FileRecorder::DestroyFileRecorder(_outputFileRecorderPtr);
    _outputFileRecorderPtr = NULL;
    _outputFileRecording = false;

    return 0;
}

    
int OutputMixer::GetMixedAudio(int sample_rate_hz,
                               int num_channels,
                               AudioFrame* frame) {
    WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                 "OutputMixer::GetMixedAudio(sample_rate_hz=%d, num_channels=%d)",
                 sample_rate_hz, num_channels);

#if 0
    if( _samplerate != _audioFrame.sample_rate_hz_ )
    {
        StopRecordingPlayout();
        _writecount = 0;
        _fileindex  = 1;
        _samplerate = _audioFrame.sample_rate_hz_;
        Initbuf(&_ringbuf, _samplerate);
    }
    
//begin addd
    char filename[256] = {0};
    if( _fileindex <= 21 )
    {
        if( Iskeyframe(_audioFrame.data_, _audioFrame.sample_rate_hz_/100, &_ringbuf ) )
        {
            if( _writecount > 600 )
            {
                StopRecordingPlayout();
                Initbuf(&_ringbuf, _samplerate);
                _writecount = 0;
            }
            else if( _writecount == 0 )
            {
                memset(_audioFrame.data_, 0, _audioFrame.samples_per_channel_*2);
                if( _audioFrame.sample_rate_hz_ == 8000 )
                    sprintf(filename, "%s/itu_mos_test_squence/dst@8k/%d_dst.wav", filenameRender_path, _fileindex);
                else if( _audioFrame.sample_rate_hz_ == 16000 )
                    sprintf(filename, "%s/itu_mos_test_squence/dst@16k/%d_dst.wav", filenameRender_path, _fileindex);
                CodecInst codecInst = {100, "L16", _audioFrame.sample_rate_hz_, _audioFrame.sample_rate_hz_/100, 1, _audioFrame.sample_rate_hz_*16};
                
                StartRecordingPlayout(filename, &codecInst);
                if( _outputFileRecording )
                    _fileindex++;
            }
        }
        else
        {
            if(_writecount == 0)
            {
                if( _audioFrame.sample_rate_hz_ == 8000 )
                    sprintf(filename, "%s/itu_mos_test_squence/dst@8k/%d_dst.wav", filenameRender_path, _fileindex);
                else if( _audioFrame.sample_rate_hz_ == 16000 )
                    sprintf(filename, "%s/itu_mos_test_squence/dst@16k/%d_dst.wav", filenameRender_path, _fileindex);
                
                CodecInst codecInst = {100, "L16", _audioFrame.sample_rate_hz_, _audioFrame.sample_rate_hz_/100, 1, _audioFrame.sample_rate_hz_*16};
                
                StartRecordingPlayout(filename, &codecInst);
#if 1
                if( _outputFileRecording )
                    _fileindex++;
#endif
            }
        }
    }
    //end added by zenggq
#endif
    // --- Record playout if enabled

    {
        CriticalSectionScoped cs(&_fileCritSect);
        if (_outputFileRecording && _outputFileRecorderPtr)
        {
            _outputFileRecorderPtr->RecordAudioToFile(_audioFrame);
            _writecount++;
        }
    }

    
    frame->num_channels_ = num_channels;
    frame->sample_rate_hz_ = sample_rate_hz;
    // TODO(andrew): Ideally the downmixing would occur much earlier, in
    // AudioCodingModule.
    RemixAndResample(_audioFrame, &resampler_, frame);
    return 0;
}
//int OutputMixer::GetMixedAudio(int sample_rate_hz,
//                               int num_channels,
//                               AudioFrame* frame) {
//  WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
//               "OutputMixer::GetMixedAudio(sample_rate_hz=%d, num_channels=%d)",
//               sample_rate_hz, num_channels);
//#if 0
//  // --- Record playout if enabled
//    //begin addd
//    if( _audioFrame.sample_rate_hz_ == 8000 )
//    {
//        CodecInst codecInst = {100, "L16", _audioFrame.sample_rate_hz_, 320, 1, 320000};
//        StartRecordingPlayout(filenameRender, &codecInst);
//    }
//    else if (_audioFrame.sample_rate_hz_ == 16000)
//    {
//        CodecInst codecInst = {100, "L16", _audioFrame.sample_rate_hz_, 960, 1, 640000};
//        StartRecordingPlayout(filenameRender, &codecInst);
//    }
//#endif
//    //end added by zenggq
//    
//  {
//    CriticalSectionScoped cs(&_fileCritSect);
//    if (_outputFileRecording && _outputFileRecorderPtr)
//      _outputFileRecorderPtr->RecordAudioToFile(_audioFrame);
//  }
//
//  frame->num_channels_ = num_channels;
//  frame->sample_rate_hz_ = sample_rate_hz;
//  // TODO(andrew): Ideally the downmixing would occur much earlier, in
//  // AudioCodingModule.
//  RemixAndResample(_audioFrame, &resampler_, frame);
//  return 0;
//}

int32_t
OutputMixer::DoOperationsOnCombinedSignal(bool feed_data_to_apm)
{
    if (_audioFrame.sample_rate_hz_ != _mixingFrequencyHz)
    {
        WEBRTC_TRACE(kTraceStream, kTraceVoice, VoEId(_instanceId,-1),
                     "OutputMixer::DoOperationsOnCombinedSignal() => "
                     "mixing frequency = %d", _audioFrame.sample_rate_hz_);
        _mixingFrequencyHz = _audioFrame.sample_rate_hz_;
    }

    // --- Insert inband Dtmf tone
    if (_dtmfGenerator.IsAddingTone())
    {
        InsertInbandDtmfTone();
    }

    // Scale left and/or right channel(s) if balance is active
    if (_panLeft != 1.0 || _panRight != 1.0)
    {
        if (_audioFrame.num_channels_ == 1)
        {
            AudioFrameOperations::MonoToStereo(&_audioFrame);
        }
        else
        {
            // Pure stereo mode (we are receiving a stereo signal).
        }

        assert(_audioFrame.num_channels_ == 2);
        AudioFrameOperations::Scale(_panLeft, _panRight, _audioFrame);
    }

    // --- Far-end Voice Quality Enhancement (AudioProcessing Module)
    if (feed_data_to_apm)
      APMAnalyzeReverseStream();

    // --- External media processing
    {
        CriticalSectionScoped cs(&_callbackCritSect);
        if (_externalMedia)
        {
            const bool is_stereo = (_audioFrame.num_channels_ == 2);
            if (_externalMediaCallbackPtr)
            {
                _externalMediaCallbackPtr->Process(
                    -1,
                    kPlaybackAllChannelsMixed,
                    (int16_t*)_audioFrame.data_,
                    _audioFrame.samples_per_channel_,
                    _audioFrame.sample_rate_hz_,
                    is_stereo);
            }
        }
    }

    // --- Measure audio level (0-9) for the combined signal
    _audioLevel.ComputeLevel(_audioFrame);

    return 0;
}

// ----------------------------------------------------------------------------
//                             Private methods
// ----------------------------------------------------------------------------

void OutputMixer::APMAnalyzeReverseStream() {
  // Convert from mixing to AudioProcessing sample rate, determined by the send
  // side. Downmix to mono.
  AudioFrame frame;
  frame.num_channels_ = 1;
  frame.sample_rate_hz_ = _audioProcessingModulePtr->input_sample_rate_hz();
  RemixAndResample(_audioFrame, &audioproc_resampler_, &frame);

  if (_audioProcessingModulePtr->AnalyzeReverseStream(&frame) == -1) {
    WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId,-1),
                 "AudioProcessingModule::AnalyzeReverseStream() => error");
  }
}

int
OutputMixer::InsertInbandDtmfTone()
{
    uint16_t sampleRate(0);
    _dtmfGenerator.GetSampleRate(sampleRate);
    if (sampleRate != _audioFrame.sample_rate_hz_)
    {
        // Update sample rate of Dtmf tone since the mixing frequency changed.
        if( _dtmfGenerator.SetSampleRate(
            (uint16_t)(_audioFrame.sample_rate_hz_) ) == -1) {
            WEBRTC_TRACE(kTraceError, kTraceVoice, VoEId(_instanceId, -1),
             "OutputMixer::InsertInbandDtmfTone() SetSampleRate %d failed.", _audioFrame.sample_rate_hz_);
            return -1;
        }
        // Reset the tone to be added taking the new sample rate into account.
        _dtmfGenerator.ResetTone();
    }

    int16_t toneBuffer[320];
    uint16_t toneSamples(0);
    if (_dtmfGenerator.Get10msTone(toneBuffer, toneSamples) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, VoEId(_instanceId, -1),
                     "OutputMixer::InsertInbandDtmfTone() inserting Dtmf"
                     "tone failed");
        return -1;
    }

    // replace mixed audio with Dtmf tone
    if (_audioFrame.num_channels_ == 1)
    {
        // mono
        memcpy(_audioFrame.data_, toneBuffer, sizeof(int16_t)
            * toneSamples);
    } else
    {
        // stereo
        for (int i = 0; i < _audioFrame.samples_per_channel_; i++)
        {
            _audioFrame.data_[2 * i] = toneBuffer[i];
            _audioFrame.data_[2 * i + 1] = 0;
        }
    }
    assert(_audioFrame.samples_per_channel_ == toneSamples);

    return 0;
}

}  // namespace voe
}  // namespace webrtc
