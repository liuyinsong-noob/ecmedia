#include "h264_record.h"
#include "Trace.h"
#include "h264_util.h"
using namespace  cloopenwebrtc;

//FILE *file_test = NULL;

static AVStream *add_video_stream(AVFormatContext *oc,  enum AVCodecID codec_id, unsigned char *data)
{
	AVStream *st = avformat_new_stream(oc, NULL);
	if (!st) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not allocate stream\n");
		return NULL;
	}

	st->id = 0;//oc->nb_streams-1;

	AVCodec codec= {0};
	codec.type= AVMEDIA_TYPE_VIDEO;


	AVCodecContext *context = st->codec;
	avcodec_get_context_defaults3( context, &codec );
	context->codec_type = AVMEDIA_TYPE_VIDEO;
	context->codec_id = AV_CODEC_ID_H264;

	context->pix_fmt = AV_PIX_FMT_YUV420P;
	st->avg_frame_rate.num = 1;
	st->avg_frame_rate.den = 10;

	st->time_base.num = 1;
	st->time_base.den = 10;
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

	return st;
}

/* Add an output stream. */
static AVStream *add_audio_stream(AVFormatContext *oc, enum AVCodecID codec_id)
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
	
    AVStream *st = avformat_new_stream(oc, audio_codec);
    if (!st) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not allocate stream\n");
		return NULL;
    }

	st->id = 1;

    AVCodecContext *c = st->codec;
	c->codec_type = AVMEDIA_TYPE_AUDIO;
	c->codec_id = AV_CODEC_ID_AAC;
	c->sample_fmt  = AV_SAMPLE_FMT_S16;
	c->bit_rate    = 16000;
	c->sample_rate = 8000;
	c->channels    = 1;

	c->profile = FF_PROFILE_AAC_MAIN;
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

    return st;
}

h264_record::h264_record(void):ai(-1)
{
	waitkey = 1;
	vi = -1;
	ai = -1;
	fc = NULL;
	AUDIO_FRAME_SIZE = 0;
	audio_frame_buf = NULL;
	audio_frame_len = 0;
	isWrited = false;
}

h264_record::~h264_record(void)
{

}

int h264_record::init(const char *filename)
{
	if(filename)
		strcpy(recordFileName, filename);

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
	if ( !fc )
		iRet = create( data, len );

	if ( fc ) {
		write_frame( data, len, timestamp);
		isWrited = true;
	}

	return iRet;
}

int h264_record::write_audio_data(short *data, int len, int freq)
{
	if (!fc || ai < 0 )
		return -1;

	int got_packet;
	int ret;
	int remainDataSize = 0;
	int remainBufSize = AUDIO_FRAME_SIZE-audio_frame_len;
	if(len >= remainBufSize) {
		remainDataSize = len - remainBufSize;
		memcpy(audio_frame_buf+audio_frame_len, data, remainBufSize*2);
		audio_frame_len = AUDIO_FRAME_SIZE;
	} else {
		memcpy(audio_frame_buf+audio_frame_len, data, len*2);
		audio_frame_len += len;
	}

	if(audio_frame_len == AUDIO_FRAME_SIZE)
	{
		//fwrite(audio_frame_buf, 1, audio_frame_len*2, file_test);

		audio_frame_len = 0;
		AVStream *pst = fc->streams[ ai ];
		AVCodecContext *c = pst->codec;

		AVFrame *frame = av_frame_alloc();
		av_frame_unref(frame);

		AVPacket pkt = { 0 }; // data and size must be 0;
		av_init_packet(&pkt);
		
		frame->nb_samples = AUDIO_FRAME_SIZE;

		ret = avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
			(uint8_t *)audio_frame_buf, AUDIO_FRAME_SIZE*2, 0);
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
		ret = av_interleaved_write_frame(fc, &pkt);
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
		memcpy(audio_frame_buf, data+(len-remainDataSize), remainDataSize*2);
		audio_frame_len = remainDataSize;
	}

	return 0 ;
}

void h264_record::write_frame( const void* p, int len, double timestamp  )
{
	if (!fc || vi < 0 )
		return;
	
	AVStream *pst = fc->streams[ vi ];

	// Init packet
	AVPacket pkt;
	av_init_packet( &pkt );
	pkt.flags |= ( 0 == getVopType( p, len ) ) ? AV_PKT_FLAG_KEY : 0;   
	pkt.stream_index = pst->index;
	pkt.data = (uint8_t*)p;
	pkt.size = len;

	// Wait for key frame
	if ( waitkey ) {
		if ( 0 == ( pkt.flags & AV_PKT_FLAG_KEY ) )
			return;
		else
			waitkey = 0;
	}

	pkt.dts = AV_NOPTS_VALUE;
	pkt.pts = AV_NOPTS_VALUE;

	if(baseH264TimeStamp == 0) {
		baseH264TimeStamp = timestamp;
	}
	AVStream *vSt = fc->streams[vi];
	AVCodecContext *avccxt = vSt->codec;
	float seconds= (timestamp - baseH264TimeStamp)/90000;
	float timebase = ((float)avccxt->time_base.num/avccxt->time_base.den);
	//算出这是第几帧
	int64_t frame = (float)seconds/timebase;
	if(frame !=0  && frame == lastFrameNum) {
		frame++;
	}
	pkt.pts = av_rescale_q(frame, vSt->codec->time_base, vSt->time_base);
	lastFrameNum = frame;

	WEBRTC_TRACE(cloopenwebrtc::kTraceInfo,
		cloopenwebrtc::kTraceVideoCoding,
		0,
		"write_frame seconds=%f timebase=%f frame=%d frame2=%d pkt.pts=%lld\n", 
		seconds, timebase, frame, lastFrameNum, (long long)pkt.pts);

	av_interleaved_write_frame( fc, &pkt );
}

void h264_record::destroy()
{
	if ( !fc )
		return;

	av_interleaved_write_frame(fc, NULL);//flushing

	av_write_trailer( fc );

	if ( fc->oformat && !( fc->oformat->flags & AVFMT_NOFILE ) && fc->pb )
		avio_close( fc->pb ); 
	
	AVStream *vSt = fc->streams[vi];
	AVCodecContext *avccxt = vSt->codec;
	free(avccxt->extradata);
	avccxt->extradata = NULL;

	// Free the stream
	 avformat_free_context( fc );

	 free(audio_frame_buf);
//	 swr_free(&swr);

	// fclose(file_test);

	fc = 0;
	waitkey = 1;
	vi = -1;
}

int h264_record::create( void *p, int len )
{
	if ( 0x67 != get_nal_type( p, len ) )
		return -1;

	//file_test = fopen("./audioSave.data", "wb");
	
	WEBRTC_TRACE(cloopenwebrtc::kTraceInfo,
		cloopenwebrtc::kTraceVideoCoding,
		0,
		"h264_record::create  fileName=%s", recordFileName);

	int ret = -1;
	av_register_all();
	avcodec_register_all();
	
	baseH264TimeStamp = 0;

	if(avformat_alloc_output_context2(&fc, NULL, NULL, recordFileName) < 0) {
			WEBRTC_TRACE(cloopenwebrtc::kTraceError,
				cloopenwebrtc::kTraceVideoCoding,
				0,
				"Could not find create context fileLen=%s", recordFileName);
			return -1;
	}

	AVOutputFormat *fmt;
	fmt = fc->oformat;
	fmt->video_codec = AV_CODEC_ID_H264;
	fmt->audio_codec = AV_CODEC_ID_AAC;

	AVStream *vPst = NULL, *aPst = NULL;

	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		vPst = add_video_stream(fc, fmt->video_codec, (unsigned char*)p);
	}
	if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		aPst = add_audio_stream(fc, fmt->audio_codec);
	}
	if(!vPst || !aPst) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"Could not create stream video=%0x audio=%0x", vPst, aPst);
		if(vPst) {
			AVCodecContext *avccxt = vPst->codec;
			free(avccxt->extradata);
			avccxt->extradata = NULL;
		}
		avformat_free_context( fc );
		fc = NULL;
		return -1;
	}
		
	int byte_per_sample = av_get_bytes_per_sample(aPst->codec->sample_fmt); 

	AUDIO_FRAME_SIZE = aPst->codec->frame_size;
	audio_frame_buf = (unsigned short*)malloc(AUDIO_FRAME_SIZE*byte_per_sample);
	audio_frame_len = 0;
	lastFrameNum = 0;

	if ( !( fc->oformat->flags & AVFMT_NOFILE ) )
		ret = avio_open( &fc->pb, fc->filename, AVIO_FLAG_READ_WRITE );

	if( ret < 0 ) {
		AVStream *vSt = fc->streams[vPst->index];
		AVCodecContext *avccxt = vSt->codec;
		free(avccxt->extradata);
		avccxt->extradata = NULL;

		// Free the stream
		avformat_free_context( fc );
		fc = 0;

		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"h264_record::create avio_open Failed");
		return -1;
	}

	avformat_write_header( fc, NULL);

	vi = vPst->index;
	ai = aPst->index;
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
	return isWrited;
}