/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This sub-API supports the following functionalities:
//
//  - Enables full duplex VoIP sessions via RTP using G.711 (mu-Law or A-Law).
//  - Initialization and termination.
//  - Trace information on text files or via callbacks.
//  - Multi-channel support (mixing, sending to multiple destinations etc.).
//
// To support other codecs than G.711, the VoECodec sub-API must be utilized.
//
// Usage example, omitting error checking:
//
//  using namespace webrtc;
//  VoiceEngine* voe = VoiceEngine::Create();
//  VoEBase* base = VoEBase::GetInterface(voe);
//  base->Init();
//  int ch = base->CreateChannel();
//  base->StartPlayout(ch);
//  ...
//  base->DeleteChannel(ch);
//  base->Terminate();
//  base->Release();
//  VoiceEngine::Delete(voe);
//
#ifndef WEBRTC_VOICE_ENGINE_VOE_BASE_H
#define WEBRTC_VOICE_ENGINE_VOE_BASE_H

#include "common_types.h"

//#include "StunMessageCallBack.h"

namespace cloopenwebrtc {
typedef int (*SoundCardOn)(int deviceType);//0, playout; 1, record
typedef int (*onReceivingDtmf)(int channelid, char dtmfch);
typedef int (*onMediaPacketTimeout)(int channelid);
typedef int (*onStunPacket)(int channelid, void *data, int len, const char *fromIP, int fromPort, bool isRTCP, bool isVideo);
typedef int (*onAudioData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
    
class AudioDeviceModule;
class AudioProcessing;
class AudioTransport;
class Config;

const int kVoEDefault = -1;

// VoiceEngineObserver
class WEBRTC_DLLEXPORT VoiceEngineObserver
{
public:
    // This method will be called after the occurrence of any runtime error
    // code, or warning notification, when the observer interface has been
    // installed using VoEBase::RegisterVoiceEngineObserver().
    virtual void CallbackOnError(int channel, int errCode) = 0;

protected:
    virtual ~VoiceEngineObserver() {}
};

// VoiceEngine
class WEBRTC_DLLEXPORT VoiceEngine
{
public:
    // Creates a VoiceEngine object, which can then be used to acquire
    // sub-APIs. Returns NULL on failure.
    static VoiceEngine* Create();
    static VoiceEngine* Create(const Config& config);

    // Deletes a created VoiceEngine object and releases the utilized resources.
    // Note that if there are outstanding references held via other interfaces,
    // the voice engine instance will not actually be deleted until those
    // references have been released.
    static bool Delete(VoiceEngine*& voiceEngine);

    // Specifies the amount and type of trace information which will be
    // created by the VoiceEngine.
    static int SetTraceFilter(unsigned int filter);

    // Sets the name of the trace file and enables non-encrypted trace messages.
    static int SetTraceFile(const char* fileNameUTF8,
                            bool addFileCounter = false);

    // Installs the TraceCallback implementation to ensure that the user
    // receives callbacks for generated trace messages.
    static int SetTraceCallback(TraceCallback* callback);

//#if !defined(WEBRTC_CHROMIUM_BUILD)
//    static int SetAndroidObjects(void* javaVM, void* env, void* context);
//#endif
	

	static int SetAndroidObjects(void* javaVM, void* env, void* context);

protected:
    VoiceEngine() {}
    ~VoiceEngine() {}
};

// VoEBase
class WEBRTC_DLLEXPORT VoEBase
{
public:
    // Factory for the VoEBase sub-API. Increases an internal reference
    // counter if successful. Returns NULL if the API is not supported or if
    // construction fails.
    static VoEBase* GetInterface(VoiceEngine* voiceEngine);

    // Releases the VoEBase sub-API and decreases an internal reference
    // counter. Returns the new reference count. This value should be zero
    // for all sub-APIs before the VoiceEngine object can be safely deleted.
    virtual int Release() = 0;

    // Installs the observer class to enable runtime error control and
    // warning notifications.
    virtual int RegisterVoiceEngineObserver(VoiceEngineObserver& observer) = 0;

    // Removes and disables the observer class for runtime error control
    // and warning notifications.
    virtual int DeRegisterVoiceEngineObserver() = 0;

    // Initializes all common parts of the VoiceEngine; e.g. all
    // encoders/decoders, the sound card and core receiving components.
    // This method also makes it possible to install some user-defined external
    // modules:
    // - The Audio Device Module (ADM) which implements all the audio layer
    // functionality in a separate (reference counted) module.
    // - The AudioProcessing module handles capture-side processing. VoiceEngine
    // takes ownership of this object.
    // If NULL is passed for any of these, VoiceEngine will create its own.
    // TODO(ajm): Remove default NULLs.
    virtual int Init(AudioDeviceModule* external_adm = NULL,
                     AudioProcessing* audioproc = NULL) = 0;

    // Returns NULL before Init() is called.
    virtual AudioProcessing* audio_processing() = 0;

    // Terminates all VoiceEngine functions and releses allocated resources.
    virtual int Terminate() = 0;

    // Creates a new channel and allocates the required resources for it.
    // One can use |config| to configure the channel. Currently that is used for
    // choosing between ACM1 and ACM2, when creating Audio Coding Module.
    virtual int CreateChannel() = 0;
    virtual int CreateChannel(const Config& config) = 0;

    // Deletes an existing channel and releases the utilized resources.
    virtual int DeleteChannel(int channel) = 0;

    // Prepares and initiates the VoiceEngine for reception of
    // incoming RTP/RTCP packets on the specified |channel|.
    virtual int StartReceive(int channel) = 0;

    // Stops receiving incoming RTP/RTCP packets on the specified |channel|.
    virtual int StopReceive(int channel) = 0;

    // Starts forwarding the packets to the mixer/soundcard for a
    // specified |channel|.
    virtual int StartPlayout(int channel) = 0;

    // Stops forwarding the packets to the mixer/soundcard for a
    // specified |channel|.
    virtual int StopPlayout(int channel) = 0;

    // Starts sending packets to an already specified IP address and
    // port number for a specified |channel|.
    virtual int StartSend(int channel) = 0;

    // Stops sending packets from a specified |channel|.
    virtual int StopSend(int channel) = 0;

    virtual int StartRecord() = 0;

    virtual int StopRecord() = 0;
    
    // Gets the version information for VoiceEngine and its components.
    virtual int GetVersion(char version[1024]) = 0;

    // Gets the last VoiceEngine error code.
    virtual int LastError() = 0;

    // TODO(xians): Make the interface pure virtual after libjingle
    // implements the interface in its FakeWebRtcVoiceEngine.
    virtual AudioTransport* audio_transport() { return NULL; }

    // To be removed. Don't use.
    virtual int SetOnHoldStatus(int channel, bool enable,
        OnHoldModes mode = kHoldSendAndPlay) { return -1; }
    virtual int GetOnHoldStatus(int channel, bool& enabled,
        OnHoldModes& mode) { return -1; }
//---begin
	virtual WebRtc_Word32 SendRaw(int channel,
            const WebRtc_Word8 *data,
            WebRtc_UWord32 length,
            bool isRTCP,
            WebRtc_UWord16 portnr = 0,
            const char *ip = NULL) = 0;

	//    Sean add begin 20131119 noise suppression
	virtual int NoiseSuppression(const void* audioSamples,
								WebRtc_Word16 *out,
								const WebRtc_UWord32 nSamples = 320,
								const WebRtc_UWord8 nBytesPerSample = 2,
								const WebRtc_UWord8 nChannels = 1,
								const WebRtc_UWord32 samplesPerSec =8000,
								const WebRtc_UWord32 totalDelayMS = 0,
								const WebRtc_Word32 clockDrift = 0,
								const WebRtc_UWord32 currentMicLevel = 0,
								const WebRtc_UWord32 mixingFrequency = 16000) = 0;
	//    Sean add end 20131119 noise suppression

	virtual int setProcessData(int channel, bool flag, bool originalFlag = false) = 0;
	virtual int pause(int channel, bool mute) = 0;

	//sean add begin 20141224 set network type
	virtual int SetNetworkType(int channelid, bool isWifi) = 0;
	//sean add end 20141224 set network type

	//
	virtual int SetSocks5SendData(int charnnel_id, unsigned char *data, int length, bool isRTCP) = 0;
	// Sets the destination port and address for a specified |channel| number.
	virtual int SetSendDestination(int channel, int rtp_port, const char *rtp_ipaddr, int sourcePort, int rtcp_port, const char *rtcp_ipaddr) = 0;
	// Gets the destination port and address for a specified |channel| number.
	virtual int GetSendDestination(int channel, int& port, char ipAddr[64],
		int& sourcePort, int& RTCPport) = 0;
	// Gets the local receiver port and address for a specified
	// |channel| number.
	virtual int GetLocalReceiver(int channel, int& port, int& RTCPport,
		char ipAddr[64]) = 0;

	// Sets the local receiver port and address for a specified
	// |channel| number.
	virtual int SetLocalReceiver(int channel, int port,
		int RTCPport = kVoEDefault,
		const char ipAddr[64] = NULL,
		const char multiCastAddr[64] = NULL) = 0;

	//Gets the socket which has been created by calling SetLocalReceiver
//	virtual int RegisterServiceCoreCallBack(int channel, ServiceCoreCallBack *messageCallBack, const char* call_id, int firewall_policy) = 0;

	//sean add begin 20140422 SetAudioGain
	virtual int setEnlargeAudioFlagOutgoing(bool flag, double factor) = 0;
	virtual int setEnlargeAudioFlagIncoming(bool flag, double factor) = 0;
	//sean add end 20140422 SetAudioGain

	//sean add begin 20140626 init and release audio device
	virtual int RegisterAudioDevice() = 0;
	virtual int DeRegisterAudioDevice() = 0;
    virtual int SetFecStatus(int channel, bool enable) = 0;
    virtual int SetLoss(int channel, int loss) = 0;
	//sean add end 20140626 init and release audio device

//---end
    virtual int SetDtmfCb(int channelid, onReceivingDtmf dtmf_cb) = 0;
    virtual int SetMediaTimeoutCb(int channelid, onMediaPacketTimeout media_timeout_cb) = 0;
    virtual int SetStunCb(int channelid, onStunPacket stun_cb) = 0;
    virtual int SetAudioDataCb(int channelid, onAudioData audio_data_cb) = 0;
    virtual int SetPCMAudioDataCallBack(int channelid, ECMedia_PCMDataCallBack audio_data_cb) = 0;
    virtual int setConferenceParticipantCallback(int channelid, ECMedia_ConferenceParticipantCallback* audio_data_cb) = 0;
//    virtual int SetSendFlag(int channelid, bool flag) = 0;
//    virtual bool GetSendFlag(int channelid) = 0;
    virtual bool  GetRecordingIsInitialized() = 0;
	virtual void* GetChannel(int channel_id) = 0;
    virtual int enableSoundTouch(int channelid, bool is_enable) = 0;
    virtual int setSoundTouch(int channelid, int pitch, int tempo, int rate) = 0;
    virtual int selectSoundTouchMode(int channelid, ECMagicSoundMode mode) = 0;
	virtual int RegisterSoundCardOnCb(SoundCardOn soundcard_on_cb) = 0;
	virtual bool GetRecordingIsRecording() = 0;
protected:
    VoEBase() {}
    virtual ~VoEBase() {}
};

}  // namespace webrtc

#endif  //  WEBRTC_VOICE_ENGINE_VOE_BASE_H
