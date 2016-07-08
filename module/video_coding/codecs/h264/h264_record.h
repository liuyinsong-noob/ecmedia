#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_RECORD_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_RECORD_H_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h" 
//#include "libswresample/swresample.h"
}


class h264_record
{
public:
	h264_record(void);
	~h264_record(void);

public:
	int init(const char *filename);
	int uninit();

	int wirte_video_data(unsigned char *data, int len, double timestamp);
	int write_audio_data(short *data, int len, int freq);


	void write_frame( const void* p, int len , double timestamp );
	void destroy();
	int create( void *p, int len );

	bool get_write_status();

	//void open_audio (AVFormatContext *oc , AVStream *st );
private:
	// < 0 = error
	// 0 = I-Frame
	// 1 = P-Frame
	// 2 = B-Frame
	// 3 = S-Frame
	int getVopType( const void *p, int len );
	int get_nal_type( void *p, int len );

private:
	char recordFileName[256];
	AVFormatContext *fc;
	
	int ai;   //audio stream index
	int vi;	//video stream index

	int waitkey;   //wait for I Frame

	int AUDIO_FRAME_SIZE;
	unsigned short* audio_frame_buf;
	int audio_frame_len;

	int64_t baseH264TimeStamp;
	int64_t lastFrameNum;
	bool  isWrited;
};

#endif