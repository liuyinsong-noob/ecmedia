#include "h264_record.h"
#include "Trace.h"
#include "h264_util.h"
using namespace  cloopenwebrtc;

//FILE *file_test = NULL;

static AVStream *add_video_stream(AVFormatContext *oc,  enum AVCodecID codec_id, unsigned char *data)
{
	AVStream *formatSt = avformat_new_stream(oc, NULL);
	if (!formatSt) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not allocate stream\n");
		return NULL;
	}

	formatSt->id = 0;//oc->nb_streams-1;

	AVCodec codec= {0};
	codec.type= AVMEDIA_TYPE_VIDEO;

	AVCodecContext *context = formatSt->codec;
	avcodec_get_context_defaults3( context, &codec );
	context->codec_type = AVMEDIA_TYPE_VIDEO;
	context->codec_id = AV_CODEC_ID_H264;

	context->pix_fmt = AV_PIX_FMT_YUV420P;
	formatSt->avg_frame_rate.num = 1;
	formatSt->avg_frame_rate.den = 10;

	formatSt->time_base.num = 1;
	formatSt->time_base.den = 10;
	context->time_base.num = 1;
	context->time_base.den = 30;

	context->extradata = (unsigned char*)malloc(34);
	memcpy(context->extradata, data, 34); //pps sps data
	context->extradata_size = 34;
	//c->bits_per_coded_sample = 0;

	SequenceParameterSet spsSet;
	memset(&spsSet, 0, sizeof(SequenceParameterSet));
	spsSet.Parse(context->extradata+4, 34);
	//c->bit_rate = 0;
	context->width = (spsSet.pic_width_in_mbs_minus1+1)*16;
	context->height = (spsSet.pic_height_in_map_units_minus1 +1)*16;

	///* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return formatSt;
}

/* Add an output stream. */
static AVStream *add_audio_stream(AVFormatContext *oc, enum AVCodecID codec_id, int freq)
{
    /* find the encoder */

	AVCodec *audio_codec = avcodec_find_encoder(codec_id);
    if (!audio_codec) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not find encoder for '%s'\n",
			avcodec_get_name(codec_id));
        return NULL;
    }
	
    AVStream *formatSt = avformat_new_stream(oc, audio_codec);
    if (!formatSt) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not allocate stream\n");
		return NULL;
    }

	formatSt->id = 1;

    AVCodecContext *c = formatSt->codec;
	c->codec_type = AVMEDIA_TYPE_AUDIO;
	c->codec_id = AV_CODEC_ID_AAC;
	c->sample_fmt  = AV_SAMPLE_FMT_S16;
	c->bit_rate    = freq*2;
	c->sample_rate = freq;
	c->channels    = 1;

	c->profile = FF_PROFILE_AAC_LOW;
	c->time_base.num = 1;
	c->time_base.den = c->sample_rate;

	//Some formats want stream headers to be separate. 
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	int ret = avcodec_open2(c, audio_codec, NULL);
	if (ret < 0) {
		char buf[128];
		av_strerror(ret, buf, 128);
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not open audio codec error=%s", buf);
		return NULL;
	}

    return formatSt;
}

h264_record::h264_record(void):audioStreamdIndex_(-1)
{
	waitkey_ = 1;
	videoStreamdIndex_ = -1;
	audioStreamdIndex_ = -1;
	formatCtxt_ = NULL;
	audioFrameSize_ = 0;
	audioFrameBuf_ = NULL;
	audioFrameLen_ = 0;
	isWrited_ = false;
	audioFreq_ = 0;
	_recordVoipCrit = CriticalSectionWrapper::CreateCriticalSection();
}

h264_record::~h264_record(void)
{
	delete _recordVoipCrit;
	_recordVoipCrit = NULL;
}

int h264_record::init(const char *filename)
{
	if(filename)
		strcpy(recordFileName_, filename);

	return 0;
}


int h264_record::uninit()
{
	destroy();
	return 0;
}

int h264_record::wirte_video_data(unsigned char *data, int len, double timestamp)
{
	int iRet = 0;
	if ( !formatCtxt_  && audioFreq_ != 0)
		iRet = create( data, len );

	if ( formatCtxt_ ) {
		write_frame( data, len, timestamp);
		isWrited_ = true;
	}

	return iRet;
}

int h264_record::write_audio_data(short *data, int len, int freq)
{
	CriticalSectionScoped lock(_recordVoipCrit);

	audioFreq_ = freq;

	if (!formatCtxt_ || audioStreamdIndex_ < 0 )
		return -1;

	int got_packet;
	int ret;
	int remainDataSize = 0;
	int remainBufSize = audioFrameSize_-audioFrameLen_;
	if(len >= remainBufSize) {
		remainDataSize = len - remainBufSize;
		memcpy(audioFrameBuf_+audioFrameLen_, data, remainBufSize*2);
		audioFrameLen_ = audioFrameSize_;
	} else {
		memcpy(audioFrameBuf_+audioFrameLen_, data, len*2);
		audioFrameLen_ += len;
	}

	if(audioFrameLen_ == audioFrameSize_)
	{
		//fwrite(audioFrameBuf_, 1, audioFrameLen_*2, file_test);

		audioFrameLen_ = 0;
		AVStream *pst = formatCtxt_->streams[ audioStreamdIndex_ ];
		AVCodecContext *c = pst->codec;

		AVFrame *frame = av_frame_alloc();
		av_frame_unref(frame);

		AVPacket pkt = { 0 }; // data and size must be 0;
		av_init_packet(&pkt);
		
		frame->nb_samples = audioFrameSize_;

		ret = avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
			(uint8_t *)audioFrameBuf_, audioFrameSize_*2, 0);
		if(ret < 0) {
			char buf[123];
			av_strerror(ret, buf, 123);
			WEBRTC_TRACE(cloopenwebrtc::kTraceError,
				cloopenwebrtc::kTraceVideoCoding,
				0,
				"Error avcodec_fill_audio_frame error=%s\n", buf);
			//av_destruct_packet(&pkt);
			av_frame_free(&frame);
			return -1;
		}

		ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
		if (ret < 0) {
			char buf[123];
			av_strerror(ret, buf, 123);
			WEBRTC_TRACE(cloopenwebrtc::kTraceError,
				cloopenwebrtc::kTraceVideoCoding,
				0,
				"Error encoding audio frame error=%s\n", buf);
			//av_destruct_packet(&pkt);
			av_frame_free(&frame);
			return -1;
		}

		av_frame_free(&frame);
		if (!got_packet) {
			//av_destruct_packet(&pkt);
			return -1;
		}

		//fwrite(pkt.data, 1, pkt.size, file_test);

		pkt.stream_index = pst->index;
		//pkt.destruct = av_destruct_packet;

		pkt.dts = AV_NOPTS_VALUE;
		pkt.pts = AV_NOPTS_VALUE;

		/* Write the compressed frame to the media file. */
		ret = av_interleaved_write_frame(formatCtxt_, &pkt);
		//av_destruct_packet(&pkt);
		if (ret != 0) {
			char buf[123];
			av_strerror(ret, buf, 123);
			WEBRTC_TRACE(cloopenwebrtc::kTraceError,
				cloopenwebrtc::kTraceVideoCoding,
				0,
				"Error while writing audio frame error=%s\n", buf);
			return -1;
		}
	}

	if(remainDataSize) {
		memcpy(audioFrameBuf_, data+(len-remainDataSize), remainDataSize*2);
		audioFrameLen_ = remainDataSize;
	}

	return 0 ;
}

void h264_record::write_frame( const void* p, int len, double timestamp  )
{
	CriticalSectionScoped lock(_recordVoipCrit);

	if (!formatCtxt_ || videoStreamdIndex_ < 0 )
		return;
	
	AVStream *pst = formatCtxt_->streams[ videoStreamdIndex_ ];

	// Init packet
	AVPacket pkt;
	av_init_packet( &pkt );
	pkt.flags |= ( 0 == getVopType( p, len ) ) ? AV_PKT_FLAG_KEY : 0;   
	pkt.stream_index = pst->index;
	pkt.data = (uint8_t*)p;
	pkt.size = len;

	// Wait for key frame
	if ( waitkey_ ) {
		if ( 0 == ( pkt.flags & AV_PKT_FLAG_KEY ) )
			return;
		else
			waitkey_ = 0;
	}

	pkt.dts = AV_NOPTS_VALUE;
	pkt.pts = AV_NOPTS_VALUE;

	if(baseH264TimeStamp_ == 0) {
		baseH264TimeStamp_ = timestamp;
	}
	AVStream *vSt = formatCtxt_->streams[videoStreamdIndex_];
	AVCodecContext *avccxt = vSt->codec;
	float seconds= (timestamp - baseH264TimeStamp_)/90000;
	float timebase = ((float)avccxt->time_base.num/avccxt->time_base.den);
	//算出这是第几帧
	int64_t frame = (float)seconds/timebase;
	if(frame !=0  && frame == lastFrameNum_) {
		frame++;
	}
	pkt.pts = av_rescale_q(frame, vSt->codec->time_base, vSt->time_base);
	lastFrameNum_ = frame;

	WEBRTC_TRACE(cloopenwebrtc::kTraceInfo,
		cloopenwebrtc::kTraceVideoCoding,
		0,
		"write_frame seconds=%f timebase=%f frame=%d frame2=%d pkt.pts=%lld\n", 
		seconds, timebase, frame, lastFrameNum_, (long long)pkt.pts);

	int ret = av_interleaved_write_frame( formatCtxt_, &pkt );
	if(ret != 0) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"av_interleaved_write_frame failed. ret=%d\n", 
			ret);
	}
}

void h264_record::destroy()
{
	CriticalSectionScoped lock(_recordVoipCrit);
	if ( !formatCtxt_ )
		return;

	av_interleaved_write_frame(formatCtxt_, NULL);//flushing
	av_write_trailer( formatCtxt_ );

	if ( formatCtxt_->oformat && !( formatCtxt_->oformat->flags & AVFMT_NOFILE ) && formatCtxt_->pb )
		avio_close( formatCtxt_->pb ); 
	
	AVStream *videoStream = formatCtxt_->streams[videoStreamdIndex_];
	AVCodecContext *videoCodec = videoStream->codec;
	free(videoCodec->extradata);
	videoCodec->extradata = NULL;
	avcodec_close(videoCodec);

	AVStream *audioStream = formatCtxt_->streams[audioStreamdIndex_];
	AVCodecContext *audioCodec = audioStream->codec;
	avcodec_close(audioCodec);

	// Free the stream
	 avformat_free_context( formatCtxt_ );
	 formatCtxt_ = NULL;

	 free(audioFrameBuf_);

	waitkey_ = 1;
	videoStreamdIndex_ = -1;
}

int h264_record::create( void *p, int len )
{
	if ( 0x67 != get_nal_type( p, len ) )
		return -1;
		
	WEBRTC_TRACE(cloopenwebrtc::kTraceInfo,
		cloopenwebrtc::kTraceVideoCoding,
		0,
		"h264_record::create  fileName=%s", recordFileName_);

	int ret = -1;
	av_register_all();
	avcodec_register_all();
	
	baseH264TimeStamp_ = 0;

	//alloc AVFormatContext
	if(avformat_alloc_output_context2(&formatCtxt_, NULL, NULL, recordFileName_) < 0) {
			WEBRTC_TRACE(cloopenwebrtc::kTraceError,
				cloopenwebrtc::kTraceVideoCoding,
				0,
				"Could not find create context fileLen=%s", recordFileName_);
			return -1;
	}

	AVOutputFormat *fmt;
	fmt = formatCtxt_->oformat;
	fmt->video_codec = AV_CODEC_ID_H264;
	fmt->audio_codec = AV_CODEC_ID_AAC;

	AVStream *videoStream = NULL, *audioStream = NULL;

	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		videoStream = add_video_stream(formatCtxt_, fmt->video_codec, (unsigned char*)p);
	}
	if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		audioStream = add_audio_stream(formatCtxt_, fmt->audio_codec, audioFreq_);
	}
	if(!videoStream || !audioStream) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not create stream video=%0x audio=%0x", videoStream, audioStream);
		if(videoStream) {
			AVCodecContext *avccxt = videoStream->codec;
			free(avccxt->extradata);
			avccxt->extradata = NULL;
			avcodec_close(avccxt);
		}

		if(audioStream) {
			AVCodecContext *audioCodec = audioStream->codec;
			avcodec_close(audioCodec);
		}

		avformat_free_context( formatCtxt_ );
		formatCtxt_ = NULL;
		return -1;
	}
		
	int byte_per_sample = av_get_bytes_per_sample(audioStream->codec->sample_fmt);
	audioFrameSize_ = audioStream->codec->frame_size;
	audioFrameBuf_ = (unsigned short*)malloc(audioFrameSize_*byte_per_sample);
	audioFrameLen_ = 0;
	lastFrameNum_ = 0;

	if ( !( formatCtxt_->oformat->flags & AVFMT_NOFILE ) )
		ret = avio_open( &formatCtxt_->pb, formatCtxt_->filename, AVIO_FLAG_READ_WRITE );

	if( ret < 0 ) {
		AVStream *vSt = formatCtxt_->streams[videoStream->index];
		AVCodecContext *avccxt = vSt->codec;
		free(avccxt->extradata);
		avccxt->extradata = NULL;
		avcodec_close(avccxt);

		AVStream *aSt = formatCtxt_->streams[audioStream->index];
		AVCodecContext *audioCodec = aSt->codec;
		avcodec_close(audioCodec);

		// Free the stream
		avformat_free_context( formatCtxt_ );
		formatCtxt_ = 0;

		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"h264_record::create avio_open Failed");
		return -1;
	}

	avformat_write_header( formatCtxt_, NULL);

	videoStreamdIndex_ = videoStream->index;
	audioStreamdIndex_ = audioStream->index;
	return 0;
}

// < 0 = error
// 0 = I-Frame
// 1 = P-Frame
// 2 = B-Frame
// 3 = S-Frame
int h264_record::getVopType( const void *p, int len )
{
	if ( !p || 6 >= len )
		return -1;

	unsigned char *b = (unsigned char*)p;

	//int aaaaaaaaaa = *b;
	//WEBRTC_TRACE(cloopenwebrtc::kTraceError,
	//	cloopenwebrtc::kTraceVideoCoding,
	//	0,
	//	"getVopType aaaaaaaaaa=%0x %0x %0x %0x %0x %0x %0x", *b, *(b+1), *(b+2), *(b+3), *(b+4), *(b+5),*(b+6));


	// Verify NAL marker
	if ( b[ 0 ] || b[ 1 ] || 0x01 != b[ 2 ] )
	{
		b++;

		if ( b[ 0 ] || b[ 1 ] || 0x01 != b[ 2 ] )
			return -1;
	} // end if

	b += 3;

	// Verify VOP id
	if ( 0xb6 == *b )
	{
		b++;
		return ( *b & 0xc0 ) >> 6;
	} // end if

	switch( *b )
	{
	case 0x65 :
	case 0x67 :
		return 0;
	case 0x61 :
		return 1;
	case 0x01 :
		return 2;
	default:
		{
			//int aaaaaaaaaa = *b;
			//WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			//	cloopenwebrtc::kTraceVideoCoding,
			//	0,
			//	"getVopType aaaaaaaaaa=%0x", aaaaaaaaaa);
		}
		break;
	} // end switch

	return -1;
}

int h264_record::get_nal_type( void *p, int len )
{
	if ( !p || 5 >= len )
		return -1;

	unsigned char *b = (unsigned char*)p;

	// Verify NAL marker
	if ( b[ 0 ] || b[ 1 ] || 0x01 != b[ 2 ] )
	{   
		b++;
		if ( b[ 0 ] || b[ 1 ] || 0x01 != b[ 2 ] )
			return -1;
	} // end if

	b += 3;

	return *b;
}

bool h264_record::get_write_status()
{
	return isWrited_;
}