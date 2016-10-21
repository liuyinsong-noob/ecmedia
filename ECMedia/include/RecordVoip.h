//
//  RecordVoip.h
//  servicecoreVideo
//
//  Created by hubin on 13-12-4.
//
//

#ifndef __servicecoreVideo__RecordVoip__
#define __servicecoreVideo__RecordVoip__

#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "list_wrapper.h"
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
#endif
#endif
{
public:
    RecordVoip();
    ~RecordVoip();

public:
	int StartRecordVoip(const char *filename);
	int StopRecordVoip(int status);

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
	virtual void Process(const int channel, const ProcessingTypes type,
                         WebRtc_Word16 audio10ms[], const int length,
                         const int samplingFreq, const bool isStereo);
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
#endif
#endif //_WIN32
#endif   

    bool isStartRecordMp4();
	bool isStartRecordWav();
	bool isStartRecordScree();
    
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
	ListWrapper _frameStorageList;
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

	char _mp4FileName[256];
    bool _startRecordMp4;

	char _recordScreenFileName[256];
	bool _startRecordScreen;
	int _recordScreenBitRates;

	int _audioDataLen;   //in short

    //RecordAudioType _recordAudioType;
    //RecordVideoType _recordVideoType;
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	h264_record  *_h264Record;
	h264_record  *_h264RecordScreen;

	H264Encoder *_h264Encoder;
	ThreadWrapper *_captureScreethread;
	CaptureScreen* _captureScreen;
#endif
#endif //_WIN32
};

#endif /* defined(__servicecoreVideo__RecordVoip__) */
