/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VOICE_ENGINE_CHANNEL_H_
#define WEBRTC_VOICE_ENGINE_CHANNEL_H_

#include "push_resampler.h"
#include "common_types.h"
#include "audio_coding_module.h"
#include "audio_conference_mixer_defines.h"
#include "rms_level.h"
#include "bitrate_controller.h"
#include "remote_ntp_time_estimator.h"
#include "rtp_header_parser.h"
#include "rtp_rtcp.h"
#include "file_player.h"
#include "file_recorder.h"
#include "../system_wrappers/include/scoped_ptr.h"
#include "dtmf_inband.h"
#include "dtmf_inband_queue.h"
#include "voe_audio_processing.h"
#include "voe_network.h"
#include "level_indicator.h"
#include "network_predictor.h"
#include "shared_data.h"
#include "voice_engine_defines.h"


#ifndef WEBRTC_EXTERNAL_TRANSPORT
#include "udp_transport.h"
#endif
#ifdef WEBRTC_SRTP
#include "SrtpModule.h"
#endif

//#include "StunMessageCallBack.h"

#ifdef WEBRTC_DTMF_DETECTION
// TelephoneEventDetectionMethods, TelephoneEventObserver
#include "voe_dtmf.h"
#endif

namespace yuntongxunwebrtc {

class TimestampWrapAroundHandler;
}

namespace yuntongxunwebrtc {

typedef int (*onReceivingDtmf)(int channelid, char dtmfch);
typedef int (*onMediaPacketTimeout)(int channelid);
typedef int (*onStunPacket)(int channelid, void *data, int len, const char *fromIP, int fromPort, bool isRTCP, bool isVideo);
typedef int (*onAudioData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
    
class AudioDeviceModule;
class Config;
class CriticalSectionWrapper;
class FileWrapper;
class ProcessThread;
class ReceiveStatistics;
class RemoteNtpTimeEstimator;
class RtpDump;
class RTPPayloadRegistry;
class RtpReceiver;
class RTPReceiverAudio;
class RtpRtcp;
class TelephoneEventHandler;
class ViENetwork;
class VoEMediaProcess;
class VoERTPObserver;
class VoiceEngineObserver;

struct CallStatistics;
struct ReportBlock;
struct SenderInfo;

namespace voe {

class Statistics;
class StatisticsProxy;
class TransmitMixer;
class OutputMixer;

// Helper class to simplify locking scheme for members that are accessed from
// multiple threads.
// Example: a member can be set on thread T1 and read by an internal audio
// thread T2. Accessing the member via this class ensures that we are
// safe and also avoid TSan v2 warnings.
class ChannelState {
 public:
    struct State {
        State() : rx_apm_is_enabled(false),
                  input_external_media(false),
                  output_file_playing(false),
                  input_file_playing(false),
                  playing(false),
                  sending(false),
                  receiving(false) {}

        bool rx_apm_is_enabled;
        bool input_external_media;
        bool output_file_playing;
        bool input_file_playing;
        bool playing;
        bool sending;
        bool receiving;
    };

    ChannelState() : lock_(CriticalSectionWrapper::CreateCriticalSection()) {
    }
    virtual ~ChannelState() {}

    void Reset() {
        CriticalSectionScoped lock(lock_.get());
        state_ = State();
    }

    State Get() const {
        CriticalSectionScoped lock(lock_.get());
        return state_;
    }

    void SetRxApmIsEnabled(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.rx_apm_is_enabled = enable;
    }

    void SetInputExternalMedia(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.input_external_media = enable;
    }

    void SetOutputFilePlaying(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.output_file_playing = enable;
    }

    void SetInputFilePlaying(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.input_file_playing = enable;
    }

    void SetPlaying(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.playing = enable;
    }

    void SetSending(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.sending = enable;
    }

    void SetReceiving(bool enable) {
        CriticalSectionScoped lock(lock_.get());
        state_.receiving = enable;
    }

private:
    scoped_ptr<CriticalSectionWrapper> lock_;
    State state_;
};

class Channel:
    public RtpData,
    public RtpFeedback,
#ifndef WEBRTC_EXTERNAL_TRANSPORT
	public UdpTransportData, // receiving packet from sockets
#else
    public TcpTransportData, // receiving packet from sockets    
#endif
    public FileCallback, // receiving notification from file player & recorder
    public Transport,
    public RtpAudioFeedback,
    public AudioPacketizationCallback, // receive encoded packets from the ACM
    public ACMVADCallback, // receive voice activity from the ACM
    public MixerParticipant // supplies output mixer with audio frames
{
public:
    enum {KNumSocketThreads = 1};
    enum {KNumberOfSocketBuffers = 8};
    virtual ~Channel();
    static int32_t CreateChannel(Channel*& channel,
                                 int32_t channelId,
                                 uint32_t instanceId,
                                 const Config& config);
    Channel(int32_t channelId, uint32_t instanceId, const Config& config);
    int32_t Init();
    int32_t SetEngineInformation(
        Statistics& engineStatistics,
        OutputMixer& outputMixer,
        TransmitMixer& transmitMixer,
        ProcessThread& moduleProcessThread,
        AudioDeviceModule& audioDeviceModule,
        VoiceEngineObserver* voiceEngineObserver,
        CriticalSectionWrapper* callbackCritSect);
    int32_t UpdateLocalTimeStamp();

    // API methods

    // VoEBase
    int32_t StartPlayout();
    int32_t StopPlayout();
    int32_t StartSend();
    int32_t StopSend();
    int32_t StartReceiving();
    int32_t StopReceiving();

    int32_t RegisterVoiceEngineObserver(VoiceEngineObserver& observer);
    int32_t DeRegisterVoiceEngineObserver();

    // VoECodec
    int32_t GetSendCodec(CodecInst& codec);
    int32_t GetRecCodec(CodecInst& codec);
    int32_t SetSendCodec(const CodecInst& codec);
    int32_t SetVADStatus(bool enableVAD, ACMVADMode mode, bool disableDTX);
    int32_t GetVADStatus(bool& enabledVAD, ACMVADMode& mode, bool& disabledDTX);
    int32_t SetRecPayloadType(const CodecInst& codec);
    int32_t GetRecPayloadType(CodecInst& codec);
    int32_t SetSendCNPayloadType(int type, PayloadFrequencies frequency);
    int SetOpusMaxPlaybackRate(int frequency_hz);

    // VoENetwork
    int32_t RegisterExternalTransport(Transport& transport);
    int32_t DeRegisterExternalTransport();
	int32_t RegisterExternalPacketization(AudioPacketizationCallback* transport);
	int32_t DeRegisterExternalPacketization();
    int32_t ReceivedRTPPacket(const int8_t* data, size_t length,const PacketTime& packet_time);
    int32_t ReceivedRTCPPacket(const int8_t* data, size_t length);

    // VoEFile
    int StartPlayingFileLocally(const char* fileName, bool loop,
                                FileFormats format,
                                int startPosition,
                                float volumeScaling,
                                int stopPosition,
                                const CodecInst* codecInst);
    int StartPlayingFileLocally(InStream* stream, FileFormats format,
                                int startPosition,
                                float volumeScaling,
                                int stopPosition,
                                const CodecInst* codecInst);
    int StopPlayingFileLocally();
    int IsPlayingFileLocally() const;
    int RegisterFilePlayingToMixer();
    int StartPlayingFileAsMicrophone(const char* fileName, bool loop,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst);
    int StartPlayingFileAsMicrophone(InStream* stream,
                                     FileFormats format,
                                     int startPosition,
                                     float volumeScaling,
                                     int stopPosition,
                                     const CodecInst* codecInst);
    int StopPlayingFileAsMicrophone();
    int IsPlayingFileAsMicrophone() const;
    int StartRecordingPlayout(const char* fileName, const CodecInst* codecInst);
    int StartRecordingPlayout(OutStream* stream, const CodecInst* codecInst);
    int StopRecordingPlayout();

    void SetMixWithMicStatus(bool mix);

    // VoEExternalMediaProcessing
    int RegisterExternalMediaProcessing(ProcessingTypes type,
                                        VoEMediaProcess& processObject);
    int DeRegisterExternalMediaProcessing(ProcessingTypes type);
    int SetExternalMixing(bool enabled);

    // VoEVolumeControl
    int GetSpeechOutputLevel(uint32_t& level) const;
    int GetSpeechOutputLevelFullRange(uint32_t& level) const;
    int SetMute(bool enable);
    bool Mute() const;
    int SetOutputVolumePan(float left, float right);
    int GetOutputVolumePan(float& left, float& right) const;
    int SetChannelOutputVolumeScaling(float scaling);
    int GetChannelOutputVolumeScaling(float& scaling) const;

    // VoENetEqStats
    int GetNetworkStatistics(NetworkStatistics& stats);
    void GetDecodingCallStatistics(AudioDecodingCallStats* stats) const;
    void enableSoundTouch(bool is_enable);
    int setSoundTouch(int pitch, int tempo, int rate);
    int selectSoundTouchMode(ECMagicSoundMode mode);
    // VoEVideoSync
    bool GetDelayEstimate(int* jitter_buffer_delay_ms,
                          int* playout_buffer_delay_ms) const;
    int least_required_delay_ms() const { return least_required_delay_ms_; }
    int SetInitialPlayoutDelay(int delay_ms);
    int SetMinimumPlayoutDelay(int delayMs);
    int GetPlayoutTimestamp(unsigned int& timestamp);
    void UpdatePlayoutTimestamp(bool rtcp);
    int SetInitTimestamp(unsigned int timestamp);
    int SetInitSequenceNumber(short sequenceNumber);

    // VoEVideoSyncExtended
    int GetRtpRtcp(RtpRtcp** rtpRtcpModule, RtpReceiver** rtp_receiver) const;

    // VoEDtmf
    int SendTelephoneEventOutband(unsigned char eventCode, int lengthMs,
                                  int attenuationDb, bool playDtmfEvent);
    int SendTelephoneEventInband(unsigned char eventCode, int lengthMs,
                                 int attenuationDb, bool playDtmfEvent);
    int SetSendTelephoneEventPayloadType(unsigned char type);
    int GetSendTelephoneEventPayloadType(unsigned char& type);

	int SetRecvTelephoneEventPayloadType(unsigned char type);
	int GetRecvTelephoneEventPayloadType(unsigned char& type);

    // VoEAudioProcessingImpl
    int UpdateRxVadDetection(AudioFrame& audioFrame);
    int RegisterRxVadObserver(VoERxVadCallback &observer);
    int DeRegisterRxVadObserver();
    int VoiceActivityIndicator(int &activity);
#ifdef WEBRTC_VOICE_ENGINE_AGC
    int SetRxAgcStatus(bool enable, AgcModes mode);
    int GetRxAgcStatus(bool& enabled, AgcModes& mode);
    int SetRxAgcConfig(AgcConfig config);
    int GetRxAgcConfig(AgcConfig& config);
#endif
#ifdef WEBRTC_VOICE_ENGINE_NR
    int SetRxNsStatus(bool enable, NsModes mode);
    int GetRxNsStatus(bool& enabled, NsModes& mode);
#endif

    // VoERTP_RTCP
    int SetLocalSSRC(unsigned int ssrc);
    int GetLocalSSRC(unsigned int& ssrc);
    int SetRemoteSSRC(unsigned int ssrc);//for distribute remote audio stream
    int GetRemoteSSRC(unsigned int& ssrc);
    int SetSendAudioLevelIndicationStatus(bool enable, unsigned char id);
    int SetReceiveAudioLevelIndicationStatus(bool enable, unsigned char id);
    int SetSendAbsoluteSenderTimeStatus(bool enable, unsigned char id);
    int SetReceiveAbsoluteSenderTimeStatus(bool enable, unsigned char id);
    void SetRTCPStatus(bool enable);
    int GetRTCPStatus(bool& enabled);
    int SetRTCP_CNAME(const char cName[256]);
    int GetRemoteRTCP_CNAME(char cName[256]);
    int GetRemoteRTCPData(unsigned int& NTPHigh, unsigned int& NTPLow,
                          unsigned int& timestamp,
                          unsigned int& playoutTimestamp, unsigned int* jitter,
                          unsigned short* fractionLost);
    int SendApplicationDefinedRTCPPacket(unsigned char subType,
                                         unsigned int name, const char* data,
                                         unsigned short dataLengthInBytes);
    int GetRTPStatistics(unsigned int& averageJitterMs,
                         unsigned int& maxJitterMs,
                         unsigned int& discardedPackets);
    int GetRemoteRTCPReportBlocks(std::vector<ReportBlock>* report_blocks);
    int GetRTPStatistics(CallStatistics& stats);
    int SetREDStatus(bool enable, int redPayloadtype);
    int GetREDStatus(bool& enabled, int& redPayloadtype);
    int SetCodecFECStatus(bool enable);
    bool GetCodecFECStatus();
    void SetNACKStatus(bool enable, int maxNumberOfPackets);
    int StartRTPDump(const char fileNameUTF8[1024], RTPDirections direction);
    int StopRTPDump(RTPDirections direction);
    bool RTPDumpIsActive(RTPDirections direction);
    // Takes ownership of the ViENetwork.
    void SetVideoEngineBWETarget(ViENetwork* vie_network, int video_channel);

    // From AudioPacketizationCallback in the ACM
    virtual int32_t SendData(
        FrameType frameType,
        uint8_t payloadType,
        uint32_t timeStamp,
        const uint8_t* payloadData,
        size_t payloadSize,
        const RTPFragmentationHeader* fragmentation) OVERRIDE;

    // From ACMVADCallback in the ACM
    virtual int32_t InFrameType(int16_t frameType) OVERRIDE;

    int32_t OnRxVadDetected(int vadDecision);

    // From RtpData in the RTP/RTCP module
    virtual int32_t OnReceivedPayloadData(
        const uint8_t* payloadData,
        size_t payloadSize,
        const WebRtcRTPHeader* rtpHeader) OVERRIDE;
    virtual bool OnRecoveredPacket(const uint8_t* packet,
                                   size_t packet_length) OVERRIDE;

    // From RtpFeedback in the RTP/RTCP module
    int32_t OnInitializeDecoder(
        const int32_t id,
        int8_t payloadType,
        const char payloadName[RTP_PAYLOAD_NAME_SIZE],
        int frequency,
        size_t channels,
        uint32_t rate);
    void OnIncomingSSRCChanged(int32_t id,
                                       uint32_t ssrc);
    void OnIncomingCSRCChanged(int32_t id,
                                       uint32_t CSRC, bool added);
    void ResetStatistics(uint32_t ssrc);

    // From RtpAudioFeedback in the RTP/RTCP module
    void OnPlayTelephoneEvent(int32_t id,
                                      uint8_t event,
                                      uint16_t lengthMs,
                                      uint8_t volume) override;

    // From Transport (called by the RTP/RTCP module)
    virtual int SendRtp(int /*channel*/,
                           const uint8_t *data,
                           size_t len, const PacketOptions* options = NULL);
    virtual int SendRtcp(int /*channel*/,
                               const uint8_t *data,
                               size_t len);

    // From MixerParticipant
    virtual int32_t GetAudioFrame(int32_t id, AudioFrame& audioFrame) OVERRIDE;
    virtual int32_t NeededFrequency(int32_t id) OVERRIDE;

    // From FileCallback
    virtual void PlayNotification(int32_t id, uint32_t durationMs) OVERRIDE;
    virtual void RecordNotification(int32_t id, uint32_t durationMs) OVERRIDE;
    virtual void PlayFileEnded(int32_t id) OVERRIDE;
    virtual void RecordFileEnded(int32_t id) OVERRIDE;

    uint32_t InstanceId() const
    {
        return _instanceId;
    }
    int32_t ChannelId() const
    {
        return _channelId;
    }
    bool Playing() const
    {
        return channel_state_.Get().playing;
    }
    bool Sending() const
    {
        return channel_state_.Get().sending;
    }
    bool Receiving() const
    {
        return channel_state_.Get().receiving;
    }
    bool ExternalTransport() const
    {
        CriticalSectionScoped cs(&_callbackCritSect);
        return _externalTransport;
    }
    bool ExternalMixing() const
    {
        return _externalMixing;
    }
    RtpRtcp* RtpRtcpModulePtr() const
    {
        return _rtpRtcpModule.get();
    }
    int8_t OutputEnergyLevel() const
    {
        return _outputAudioLevel.Level();
    }
    //uint32_t Demultiplex(const AudioFrame& audioFrame);
	WebRtc_UWord32 Demultiplex(const AudioFrame& audioFrame, const AudioFrame& audioFrame2Up);
    // Demultiplex the data to the channel's |_audioFrame|. The difference
    // between this method and the overloaded method above is that |audio_data|
    // does not go through transmit_mixer and APM.
    void Demultiplex(const int16_t* audio_data,
                     int sample_rate,
                     int number_of_frames,
                     int number_of_channels);
    uint32_t PrepareEncodeAndSend(int mixingFrequency);
    uint32_t EncodeAndSend();

    // From BitrateObserver (called by the RTP/RTCP module).
    void OnNetworkChanged(const uint32_t bitrate_bps,
                          const uint8_t fraction_lost,  // 0 - 255.
                          const int64_t rtt);
#ifndef WEBRTC_EXTERNAL_TRANSPORT
    int32_t SetUdpTransport(UdpTransport *transport, int32_t rtp_port);
    UdpTransport *GetUdpTransport();
#else
	int32_t SetTcpTransport(TcpTransport *transport, int32_t rtp_port);
	TcpTransport *GetTcpTransport();
#endif
    
private:
    bool ReceivePacket(const uint8_t* packet, size_t packet_length,
                       const RTPHeader& header, bool in_order);
    bool HandleEncapsulation(const uint8_t* packet,
                             size_t packet_length,
                             const RTPHeader& header);
    bool IsPacketInOrder(const RTPHeader& header) const;
    bool IsPacketRetransmitted(const RTPHeader& header, bool in_order) const;
    int ResendPackets(const uint16_t* sequence_numbers, int length);
    int InsertInbandDtmfTone();
    int32_t MixOrReplaceAudioWithFile(int mixingFrequency);
    int32_t MixAudioWithFile(AudioFrame& audioFrame, int mixingFrequency);
    int32_t SendPacketRaw(const void *data, size_t len, bool RTCP);
    void UpdatePacketDelay(uint32_t timestamp,
                           uint16_t sequenceNumber);
    void RegisterReceiveCodecsToRTPModule();

    int SetRedPayloadType(int red_payload_type);
    int SetSendRtpHeaderExtension(bool enable, RTPExtensionType type,
                                  unsigned char id);

    int32_t GetPlayoutFrequency();
    int64_t GetRTT() const;

    CriticalSectionWrapper& _fileCritSect;
    CriticalSectionWrapper& _callbackCritSect;
    CriticalSectionWrapper& volume_settings_critsect_;
    uint32_t _instanceId;
    int32_t _channelId;

    ChannelState channel_state_;

    scoped_ptr<RtpHeaderParser> rtp_header_parser_;
    scoped_ptr<RTPPayloadRegistry> rtp_payload_registry_;
    scoped_ptr<ReceiveStatistics> rtp_receive_statistics_;
    scoped_ptr<StatisticsProxy> statistics_proxy_;
    scoped_ptr<RtpReceiver> rtp_receiver_;
    TelephoneEventHandler* telephone_event_handler_;
    scoped_ptr<RtpRtcp> _rtpRtcpModule;
    scoped_ptr<AudioCodingModule> audio_coding_;
    RtpDump& _rtpDumpIn;
    RtpDump& _rtpDumpOut;
    AudioLevel _outputAudioLevel;
    bool _externalTransport;
    AudioFrame _audioFrame;
    scoped_ptr<int16_t[]> mono_recording_audio_;
    // Downsamples to the codec rate if necessary.
    PushResampler<int16_t> input_resampler_;
    FilePlayer* _inputFilePlayerPtr;
    FilePlayer* _outputFilePlayerPtr;
    FileRecorder* _outputFileRecorderPtr;
    int _inputFilePlayerId;
    int _outputFilePlayerId;
    int _outputFileRecorderId;
    bool _outputFileRecording;
    DtmfInbandQueue _inbandDtmfQueue;
    DtmfInband _inbandDtmfGenerator;
    bool _outputExternalMedia;
    VoEMediaProcess* _inputExternalMediaCallbackPtr;
    VoEMediaProcess* _outputExternalMediaCallbackPtr;
    uint32_t _timeStamp;
    uint8_t _sendTelephoneEventPayloadType;
	uint8_t _recvTelephoneEventPayloadType;

    RemoteNtpTimeEstimator ntp_estimator_ GUARDED_BY(ts_stats_lock_);

    // Timestamp of the audio pulled from NetEq.
    uint32_t jitter_buffer_playout_timestamp_;
    uint32_t playout_timestamp_rtp_;
    uint32_t playout_timestamp_rtcp_;
    uint32_t playout_delay_ms_;
    uint32_t _numberOfDiscardedPackets;
    uint16_t send_sequence_number_;
    uint8_t restored_packet_[kVoiceEngineMaxIpPacketSizeBytes];

    scoped_ptr<CriticalSectionWrapper> ts_stats_lock_;

    scoped_ptr<TimestampWrapAroundHandler> rtp_ts_wraparound_handler_;
    // The rtp timestamp of the first played out audio frame.
    int64_t capture_start_rtp_time_stamp_;
    // The capture ntp time (in local timebase) of the first played out audio
    // frame.
    int64_t capture_start_ntp_time_ms_ GUARDED_BY(ts_stats_lock_);

    // uses
    Statistics* _engineStatisticsPtr;
    OutputMixer* _outputMixerPtr;
    TransmitMixer* _transmitMixerPtr;
    ProcessThread* _moduleProcessThreadPtr;
    AudioDeviceModule* _audioDeviceModulePtr;
    VoiceEngineObserver* _voiceEngineObserverPtr; // owned by base
    CriticalSectionWrapper* _callbackCritSectPtr; // owned by base
    Transport* _transportPtr; // WebRtc socket or external transport
    RMSLevel rms_level_;
    scoped_ptr<AudioProcessing> rx_audioproc_; // far end AudioProcessing
    VoERxVadCallback* _rxVadObserverPtr;
    int32_t _oldVadDecision;
    int32_t _sendFrameType; // Send data is voice, 1-voice, 0-otherwise
    // VoEBase
    bool _externalMixing;
    bool _mixFileWithMicrophone;
    // VoEVolumeControl
    bool _mute;
    float _panLeft;
    float _panRight;
    float _outputGain;
    // VoEDtmf
    bool _playOutbandDtmfEvent;
    bool _playInbandDtmfEvent;
    // VoeRTP_RTCP
    uint32_t _lastLocalTimeStamp;
    int8_t _lastPayloadType;
    bool _includeAudioLevelIndication;
    // VoENetwork
    AudioFrame::SpeechType _outputSpeechType;
    ViENetwork* vie_network_;
    int video_channel_;
    // VoEVideoSync
    uint32_t _average_jitter_buffer_delay_us;
    int least_required_delay_ms_;
    uint32_t _previousTimestamp;
    uint16_t _recPacketDelayMs;
    // VoEAudioProcessing
    bool _RxVadDetection;
    bool _rxAgcIsEnabled;
    bool _rxNsIsEnabled;
    bool restored_packet_in_use_;
    // RtcpBandwidthObserver
    scoped_ptr<BitrateController> bitrate_controller_;
    scoped_ptr<RtcpBandwidthObserver> rtcp_bandwidth_observer_;
    scoped_ptr<BitrateObserver> send_bitrate_observer_;
    scoped_ptr<NetworkPredictor> network_predictor_;

//----begin
private:
    onReceivingDtmf _dtmf_cb;
    onMediaPacketTimeout _media_timeout_cb;
    onStunPacket _stun_cb;
    onAudioData _audio_data_cb;
    ECMedia_PCMDataCallBack _audio_pcm_callback;
public:
    int setDtmfCb(onReceivingDtmf dtmf_cb);
    int setMediaTimeoutCb(onMediaPacketTimeout media_timeout_cb);
    int setStunCb(onStunPacket dtmf_cb);
    int setAudioDataCb(onAudioData dtmf_cb);
    int SetPCMAudioDataCallBack(ECMedia_PCMDataCallBack callback);
private:
//	ServiceCoreCallBack *_serviceCoreCallBack;
	char call_id[9];
	int _firewall_policy;

	bool _processDataFlag;
	bool _processOriginalDataFlag;
	void *_sendData;
	void *_receiveData;
	void *_sendOriginalData;
	void *_receiveOriginalData;
	//    sean add begin 20140708 original audio sample
	AudioFrame _audioFrame2Up;
	//    sean add end 20140708 original audio sample

	bool _encrypting;
	bool _decrypting;
	Encryption* _encryptionPtr; // WebRtc SRTP or external encryption
	WebRtc_UWord8* _encryptionRTPBufferPtr;
	WebRtc_UWord8* _decryptionRTPBufferPtr;
	WebRtc_UWord8* _encryptionRTCPBufferPtr;
	WebRtc_UWord8* _decryptionRTCPBufferPtr;
#ifdef WEBRTC_SRTP
	SrtpModule& _srtpModule;
#endif

	//add by fly
	WebRtc_UWord32 _dtmfTimeStampRTP;
	WebRtc_UWord32 _lastdtmfTimeStampRTP;
	bool _ondtmf;

#ifndef WEBRTC_EXTERNAL_TRANSPORT
	WebRtc_UWord8 _numSocketThreads;
	UdpTransport* _socketTransportModule;
#else
    WebRtc_UWord8 _numSocketThreads;
    TcpTransport* _socketTransportModule;    
#endif

	time_t  _startNetworkTime;
	scoped_ptr<CriticalSectionWrapper> critsect_net_statistic;

	//sean add begin 20141224 set network type
	bool _isWifi;
	long long _sendDataTotalSim;
	long long _recvDataTotalSim;
	long long _sendDataTotalWifi;
	long long _recvDataTotalWifi;

	bool _pause;
    int _loss;
public:
    int SetLoss(int loss);
	int setNetworkType(bool isWifi);
	//sean add end 20141224 set network type
//	WebRtc_Word32 RegisterServiceCoreCallBack(ServiceCoreCallBack *, const char*, int firewall_policy);
//	WebRtc_Word32 DeRegisterServiceCoreCallBack();
	int setProcessData(bool flag, bool originalFlag = false);
	//Sean add begin 20140322 for window XP
	WebRtc_UWord32 pause(bool mute);
	//Sean add end 20140322 for window XP

//#ifndef WEBRTC_EXTERNAL_TRANSPORT
	WebRtc_Word32 SetLocalReceiver(const WebRtc_UWord16 rtpPort,
		const WebRtc_UWord16 rtcpPort,
		const char ipAddr[64],
		const char multicastIpAddr[64]);
	WebRtc_Word32 GetLocalReceiver(int& port, int& RTCPport, char ipAddr[]);
    WebRtc_Word32 SetSocks5SendData(unsigned char *data, int length, bool isRTCP);
	WebRtc_Word32 SetSendDestination(const WebRtc_UWord16 rtpPort, const char rtp_ipAddr[64], const int sourcePort, const WebRtc_UWord16 rtcpPort, const char rtcp_ipAddr[64]);
	WebRtc_Word32 GetSendDestination(int& port, char ipAddr[64],
		int& sourcePort, int& RTCPport);
//#endif

#ifdef WEBRTC_SRTP
	int CcpSrtpInit();
	int CcpSrtpShutdown();
	int EnableSRTPSend(ccp_srtp_crypto_suite_t crypt_type, const char* key);
	int DisableSRTPSend();
	int EnableSRTPReceive(ccp_srtp_crypto_suite_t crypt_type, const char* key);
	int DisableSRTPReceive();
#endif
	int RegisterExternalEncryption(Encryption& encryption);
	int DeRegisterExternalEncryption();

	//add by fly
	bool handleRFC2833(const WebRtc_Word8* packet, const WebRtc_Word32 packetlen);

#ifndef   WEBRTC_EXTERNAL_TRANSPORT
	UdpTransport* GetSocketTransportModule() const
	{
		return _socketTransportModule;
	}
#else
    TcpTransport* GetSocketTransportModule() const
    {
        return _socketTransportModule;
    }    
#endif

	// From UdpTransportData in the Socket Transport module
	void IncomingRTPPacket(const WebRtc_Word8* incomingRtpPacket,
		const WebRtc_Word32 rtpPacketLength,
		const char* fromIP,
		const WebRtc_UWord16 fromPort);

	void IncomingRTCPPacket(const WebRtc_Word8* incomingRtcpPacket,
		const WebRtc_Word32 rtcpPacketLength,
		const char* fromIP,
		const WebRtc_UWord16 fromPort);

	void getNetworkStatistic(time_t &startTime,  long long &sendLengthSim,  long long &recvLengthSim, long long &sendLengthWifi,  long long &recvLengthWifi);


	WebRtc_Word32 SendUDPPacket(const void* data, unsigned int length,
		int& transmittedBytes, bool useRtcpSocket);
    
    int setConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback* cb);
    int setConferenceParticipantCallbackTimeInterVal(int timeInterVal);
//#ifndef WEBRTC_EXTERNAL_TRANSPORT
	WebRtc_Word32 GetSourceInfo(int& rtpPort, int& rtcpPort, char ipAddr[64]);
	WebRtc_Word32 EnableIPv6();
	bool IPv6IsEnabled() const;
	WebRtc_Word32 SetSourceFilter(int rtpPort, int rtcpPort,
		const char ipAddr[64]);
	WebRtc_Word32 GetSourceFilter(int& rtpPort, int& rtcpPort, char ipAddr[64]);
	WebRtc_Word32 SetSendTOS(int DSCP, int priority, bool useSetSockopt);
	WebRtc_Word32 GetSendTOS(int &DSCP, int& priority, bool &useSetSockopt);
#if defined(_WIN32)
	WebRtc_Word32 SetSendGQoS(bool enable, int serviceType, int overrideDSCP);
	WebRtc_Word32 GetSendGQoS(bool &enabled, int &serviceType,
		int &overrideDSCP);
#endif
//#endif

//#ifndef WEBRTC_EXTERNAL_TRANSPORT
	bool SendSocketsInitialized() const
	{
		return _socketTransportModule->SendSocketsInitialized();
	}
	bool ReceiveSocketsInitialized() const
	{
		return _socketTransportModule->ReceiveSocketsInitialized();
	}
//#endif
	//---end
public:
    WebRtc_Word32 SetPacketTimeoutNotification(bool enable, int timeoutSeconds);
    WebRtc_Word32 GetPacketTimeoutNotification(bool& enabled,
                                               int& timeoutSeconds);
    virtual void OnPacketTimeout(const WebRtc_Word32 id);
    
    virtual void OnReceivedPacket(const WebRtc_Word32 id,
                          const RtpRtcpPacketType packetType);

public:
	int32_t SetKeepAliveStatus(const bool enable,
		const int8_t unknownPayloadType,
		const uint16_t deltaTransmitTimeMS);

	int32_t GetKeepAliveStatus(bool& enable,
		int8_t& unknownPayloadType,
		uint16_t& deltaTransmitTimeMS);
    int8_t SetMixMediaStream(bool enable, char *mixture, unsigned char version);

private:
    bool _rtpPacketTimedOut;
    bool _rtpPacketTimeOutIsEnabled;
    WebRtc_UWord32 _rtpTimeOutSeconds;
    int _rtp_port;
    unsigned int _remote_ssrc;//for distribute remote audio stream
};

}  // namespace voe
}  // namespace webrtc

#endif  // WEBRTC_VOICE_ENGINE_CHANNEL_H_
