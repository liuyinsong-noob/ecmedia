//
//  RecordVoip.h
//  servicecoreVideo
//
//  Created by hubin on 13-12-4.
//
//

#ifndef __servicecoreVideo__RecordVoip__
#define __servicecoreVideo__RecordVoip__

#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "../system_wrappers/include/list_wrapper.h"
#include "voe_external_media.h"

#ifdef _WIN32
#ifdef VIDEO_ENABLED
#include "video_coding_defines.h"
#include "h264.h"
#include "CaptureScreen.h"
#include "h264_record.h"
#endif
#endif

using namespace cloopenwebrtc;

class RecordVoip
		: public VoEMediaProcess
#ifdef _WIN32
#ifdef VIDEO_ENABLED
		, public VCMFrameStorageCallback
		, public EncodedImageCallback
		, public VCMPacketizationCallback
#endif
#endif
{
public:
    RecordVoip();
    ~RecordVoip();

public:
	int StartRecordRemoteVideo(const char *filename);
	int StopRecordRemoteVideo(int status);

	int StartRecordLocalVideo(const char *filename);
	int StopRecordLocalVideo(int status);

	int StartRecordAudio(const char *filename);
	int StartRecordAudioEx(const char *rFileName, const char *lFileName);
	int StopRecordAudio(int status);

	int StartRecordScreen(const char *filename, int bitrates, int fps, int screenIndex);
	int StartRecordScreenEx(const char *filename, int bitrates, int fps, int screenIndex, int left, int top, int width, int height);
	int StopRecordScreen(int status);
    
public:
	static bool RecordVideoThreadRun(void* obj);
    static bool RecordAudioThreadRun(void* obj);
	static bool CaptureScreenThreadRun(void* obj);

	bool ProcessVideoData();
	bool ProcessAudioData();

public:
#if !defined(NO_VOIP_FUNCTION)
    //implement VoEMediaProcess
	virtual void Process(int channel, ProcessingTypes type,
                         WebRtc_Word16 audio10ms[], int length,
                         int samplingFreq, bool isStereo);
#ifdef _WIN32
#ifdef VIDEO_ENABLED
    //implement VCMFrameStorageCallback
    virtual WebRtc_Word32 StoreReceivedFrame(const EncodedVideoData& frameToStore);
	WebRtc_Word32 CapturedScreeImage(unsigned char *imageData, int size, int width, int height);
	//WebRtc_Word32 StoreScreenFrame(const EncodedVideoData& frameToStore);

	virtual int32_t Encoded(
		const EncodedImage& encoded_image,
		const CodecSpecificInfo* codec_specific_info = NULL,
		const RTPFragmentationHeader* fragmentation = NULL);

	//implement VCMPacketizationCallback
	virtual int32_t SendData(uint8_t payloadType,
		const EncodedImage& encoded_image,
		const RTPFragmentationHeader& fragmentationHeader,
		const RTPVideoHeader* rtpVideoHdr);
#endif
#endif //_WIN32
#endif   
	
	bool isStartRecordRVideo() { return _startRecordRVideo; }
	bool isStartRecordLVideo() { return _startRecordLVideo; }
	bool isStartRecordWav() { return _startRecordWav; }
	bool isStartRecordScree() { return _startRecordScreen; }

	bool isRecording() { return _startRecordRVideo || _startRecordLVideo || _startRecordWav || _startRecordScreen; };
    
	bool isAlreadWriteScreenAudio();
private:
	int WriteWavFileHeader(FILE *file);
	int CompleteWavFile(FILE *file);

	int StopAudioFile(FILE *audioFile, const char *filename,  int status);

public:
		int _captureScreenInterval;
		int _captureScreenIndex;
		int _recordScreenLeft;
		int _recordScreenTop;
		int _recordScreenWidth;
		int _recordScreenHeight;
		int _audioSamplingFreq;

private:
    ThreadWrapper *_videoThread;
	ThreadWrapper *_audioThread;

	EventWrapper *_videoEvent;
    EventWrapper *_audioEvent;

    ListWrapper _playbackList;
    ListWrapper _recordingList;
	ListWrapper _frameRecvList;
	ListWrapper _frameSendList;
	ListWrapper _frameScreenList;

	CriticalSectionWrapper* _videoCrit;
	CriticalSectionWrapper* _audioCrit;

    CriticalSectionWrapper* _critsectH264Encoder;

	char _wavFileName[256];
	char _wavRemoteFileName[256];
	char _wavLocalFileName[256];

	FILE *_wavRecordFile;
	FILE *_wavRemoteRecordFile;
	FILE *_wavLocalRecordFile;

	bool _startRecordWav;

	char _remoteVideoFileName[256];
    bool _startRecordRVideo;

	char _localVideoFileName[256];
	bool _startRecordLVideo;

	char _recordScreenFileName[256];
	bool _startRecordScreen;
	int _recordScreenBitRates;
	int _recordScreenFps;

	int _audioDataLen;   //in short

    //RecordAudioType _recordAudioType;
    //RecordVideoType _recordVideoType;
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	h264_record  *_h264RecordRemote;
	h264_record  *_h264RecordLocal;
	h264_record  *_h264RecordScreen;

	H264Encoder *_h264Encoder;
	ThreadWrapper *_captureScreethread;
	CaptureScreen* _captureScreen;
#endif
#endif //_WIN32
};

#endif /* defined(__servicecoreVideo__RecordVoip__) */
