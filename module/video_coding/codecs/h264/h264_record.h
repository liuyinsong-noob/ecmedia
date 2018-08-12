#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_RECORD_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_RECORD_H_

extern "C"
{
#ifdef __APPLE__
#include "libavcodec_ios/avcodec.h"
#include "libavformat_ios/avformat.h"
#include "libavutil_ios/opt.h"
#else
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#endif // endif __APPLE__
//#include "libswresample/swresample.h"
}
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/clock.h"

using namespace yuntongxunwebrtc;

class h264_record
{
public:
	h264_record(void);
	~h264_record(void);

public:
	int init(const char *filename);
	int uninit();

	int wirte_video_data(unsigned char *data, int len, uint32_t timestamp);
	int write_audio_data(short *data, int len, int freq);


	void write_video_frame(const void *p, int len, uint32_t timestamp);
	void destroy();
	int create( void *p, int len);

	bool get_write_status();

	int get_audio_freq() { return audioFreq_; }
	//void open_audio (AVFormatContext *oc , AVStream *st );
private:
	// < 0 = error
	// 0 = I-Frame
	// 1 = P-Frame
	// 2 = B-Frame
	// 3 = S-Frame
	int getVopType( const void *p, int len );
	int get_nal_type( void *p, int len );
	void pcm_s16le_to_s16be(short *data, int len);

private:
	CriticalSectionWrapper* _recordVoipCrit;

	char recordFileName_[256];
	AVFormatContext *formatCtxt_;
	
	int audioStreamdIndex_;   //audio stream index
	int videoStreamdIndex_;	//video stream index

	int waitkey_;   //wait for I Frame

	int audioFrameSize_;
	unsigned short* audioFrameBuf_;
	int audioFrameLen_;
	int audioFreq_;

	uint32_t baseH264TimeStamp_;
	uint32_t lastVideoFrameNum_;
	bool  isWrited_;
	
	Clock* clock_;
	int64_t baseAudioTime_;
	int64_t lastAudioFrameNum_;
};

#endif
