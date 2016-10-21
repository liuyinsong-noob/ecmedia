//
//  H264_dec.cc
//  video_coding
//
//  Created by Lee Sean on 13-1-30.

#include "H264_dec.h"
#include "Trace.h"
#include "h264_util.h"
//#include "h264_record.h"

//#define DEBUG_FRAGMENT_FILE
#include "module_common_types.h"

#include <string.h>
#if 0
char *globalFilePath = NULL;
char *globalFilePath2 = NULL;
#endif
int reinitDecoderFlag = 0;

namespace cloopenwebrtc
{
    void H264Decoder::pgm_save(FILE *f,unsigned char *buf,int wrap, int xsize,int ysize)
    {
        //pgm_save(fout,pFrame_->data[0], pFrame_->linesize[0],
//        codec_->width, codec_->height);
        if (!f) {
            return;
        }
        int i;
#ifdef  WEBRTC_ANDROID
	   flockfile(f);
#endif

        for(i=0;i<ysize;i++)
        {
            fwrite(buf + i * wrap, 1, xsize, f );
        }
#ifdef  WEBRTC_ANDROID
       funlockfile(f);
#endif
    }
    
H264Decoder* H264Decoder::Create() {
    return new H264Decoder();
}
    
H264Decoder::H264Decoder():
_decodedImage(),
_inited(false),
_decodeCompleteCallback(NULL),
_codecContext(NULL),
pFrame_(NULL)
#if 0
,
fout(NULL),
fout2(NULL)
#endif
{
    memset((void*)&_decodedImage, 0, sizeof(_decodedImage));
#if defined(DEBUG_FRAGMENT_FILE) and defined(_WIN32)
	_fragFIle = fopen("frag.264","wb");
#endif
}
    
H264Decoder::~H264Decoder()
{
    Release();
#if defined(DEBUG_FRAGMENT_FILE) and defined(_WIN32)
	fclose(_fragFIle);
#endif
}
    
WebRtc_Word32
H264Decoder::Reset()
{
    return WEBRTC_VIDEO_CODEC_OK;
}
    
WebRtc_Word32
H264Decoder::InitDecode(const VideoCodec* codecSettings, WebRtc_Word32 numberOfCores )
{
	int ret = 0;
#if 0
    fout = fopen(globalFilePath, "ab+");
    WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"sean globalFilePath:%s!!!",globalFilePath);
    if (globalFilePath)
        fout = fopen(globalFilePath, "ab+");
    if (!fout)
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"sean openfile failed!!!");
    if (globalFilePath2)
        fout2 = fopen(globalFilePath2, "ab+");
    if (!fout2)
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"sean openfile2 failed!!!");
#endif
    if (codecSettings == NULL
		|| (codecSettings->width < 1)
		|| (codecSettings->height < 1))
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

	_decoderSetting = *codecSettings;
	_numberOfCores	= numberOfCores;

        
    //初始化h264解码库
    avcodec_register_all();
        
    /* find the video encoder */
    AVCodec *decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!decoder)
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"CODEC NOT FOUND!!!");
        return -1;
    }
    _codecContext = avcodec_alloc_context3(decoder);

    if (NULL == _codecContext) {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"CANNOT ALLOC CONTEXT!");
        return -1;
    }
    ret = avcodec_open2(_codecContext, decoder, NULL);
    if(ret >= 0)
        pFrame_ = av_frame_alloc();// Allocate video frame
    else
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"CANNOT OPEN CODEC %d!",ret);
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

	_inited = true;
        
    return WEBRTC_VIDEO_CODEC_OK;
}
    
void H264Decoder::reInitDec()
{
	AVCodec *videoCodec;
	int error;
    if (_codecContext) {
        avcodec_close(_codecContext);
    }  
    videoCodec=avcodec_find_decoder(AV_CODEC_ID_H264);
    if (videoCodec==NULL)
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"Could not find H264 decoder in ffmpeg.");

    avcodec_get_context_defaults3(_codecContext,videoCodec);
    error = avcodec_open2(_codecContext, videoCodec, NULL);
    if (error!=0){
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"avcodec_open() failed.");
    }
}
    
WebRtc_Word32
H264Decoder::Decode(const EncodedImage& inputImage,
                    bool /*missingFrames*/,
                    const RTPFragmentationHeader* fragmentation,
                    const CodecSpecificInfo* /*codecSpecificInfo*/,
                    WebRtc_Word64 /*renderTimeMs*/)
{
    if (inputImage._buffer == NULL
		|| inputImage._length <= 0)
    {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (_decodeCompleteCallback == NULL
		|| !_inited)
    {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
 
        
    if (1 == reinitDecoderFlag) {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"H264Decoder:: sean reinit decoder called");
        reInitDec();
        reinitDecoderFlag = 0;
    }


	AVPacket packet = {0};
	int frameFinished = 0;
#ifndef FRAGMENT_DECODE
	{
        int num=0;
#if defined(DEBUG_FRAGMENT_FILE) and defined(_WIN32)
        int i=0;
		if (fragmentation->fragmentationVectorSize > 0)
		{
			while (num < fragmentation->fragmentationVectorSize)
			{
				int offset = fragmentation->fragmentationOffset[num];
				int len = fragmentation->fragmentationLength[num];
				fwrite(inputImage._buffer+offset,sizeof(uint8_t), len, _fragFIle);
				num++;
			}
		}else{
			fwrite(inputImage._buffer,sizeof(uint8_t), inputImage._length, _fragFIle);
		}
		
#endif
		num = fragmentation->fragmentationVectorSize;
		int ret;
		for (int i=0; i<num; i++)
		{
			packet.size += fragmentation->fragmentationLength[i];
		}
		{	
			if (fragmentation->fragmentationVectorSize > 0)
			{
				packet.data = inputImage._buffer+fragmentation->fragmentationOffset[0];
			}else
			{
				packet.data = inputImage._buffer;
				packet.size = inputImage._length;//
			}
			
			
			ret = avcodec_decode_video2(_codecContext, pFrame_, &frameFinished, &packet);

			if(frameFinished)//成功解码
			{            
				return ReturnFrame(pFrame_, inputImage._timeStamp, inputImage.ntp_time_ms_);
				
			}else
				return WEBRTC_VIDEO_CODEC_ERROR;  //
			
		}
	}
#else
    int ret = avcodec_decode_video2(_codecContext, pFrame_, &frameFinished, &packet);
    if(ret < 0)
    {
        WEBRTC_TRACE(cloopenwebrtc::kTraceError,cloopenwebrtc::kTraceVideoCoding,0,"H264Decoder::Decode Failed ret=%d", ret);
        return ret;
    }
#endif
    
}
    
WebRtc_Word32
H264Decoder::RegisterDecodeCompleteCallback(DecodedImageCallback* callback)
{
    _decodeCompleteCallback = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}
    
WebRtc_Word32
H264Decoder::Release()
{
#if 0
    if (fout) {
        fclose(fout);
        fout = NULL;
    }
    if (fout2) {
        fclose(fout2);
        fout2 = NULL;
    }
#endif
    //if (_decodedImage._buffer != NULL)
    //{
    //    delete [] _decodedImage._buffer;
    //    _decodedImage._buffer = NULL;
    //}
    _inited = false;
    if (_codecContext) {
        avcodec_close(_codecContext);
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

// < 0 = error
// 0 = I-Frame
// 1 = P-Frame
// 2 = B-Frame
// 3 = S-Frame
int H264Decoder::getVopType( const void *p, int len )
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

int H264Decoder::get_nal_type( void *p, int len )
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
int H264Decoder::PrepareRawImage(AVFrame *pFrame)
{
//	int a=0,i;
	_decodedImage.set_width( pFrame->width);
	_decodedImage.set_height( pFrame->height);

	//_decodedImage._length = 0;

	/*for (i=0; i<pFrame->height; i++)
	{
		memcpy(_decodedImage._buffer+a,pFrame->data[0] + i * pFrame->linesize[0], pFrame->width);
		a+=pFrame->width;
	}
	for (i=0; i<pFrame->height/2; i++)
	{
		memcpy(_decodedImage._buffer+a,pFrame_->data[1] + i * pFrame_->linesize[1], pFrame->width/2);
		a+=pFrame->width/2; 
	} 
	for (i=0; i<pFrame->height/2; i++) 
	{ 
		memcpy(_decodedImage._buffer+a,pFrame_->data[2] + i * pFrame_->linesize[2], pFrame->width/2); 
		a+=pFrame->width/2; 
	}*/
	return 0;
}

int H264Decoder::ReturnFrame(const AVFrame* img, uint32_t timeStamp, int64_t ntp_time_ms)
{
	I420VideoFrame decodedImage;
	int i = decodedImage.allocated_size(kYPlane);//
	if (img == NULL) {
		// Decoder OK and NULL image => No show frame
		return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
	}
	// Allocate memory for decoded image.
	int size_y = img->linesize[0]*img->height;          
	int size_u = img->linesize[1]*(img->height+1)/2;
	int size_v = img->linesize[2]*(img->height+1)/2;
	// TODO(mikhal): This does  a copy - need to SwapBuffers.
	decodedImage.CreateFrame(size_y, img->data[0],
		size_u, img->data[1],
		size_v, img->data[2],
		img->width, img->height,
		img->linesize[0],
		img->linesize[1],
		img->linesize[2]);
	decodedImage.set_timestamp(timeStamp);
	decodedImage.set_ntp_time_ms(ntp_time_ms);
    //printf("decode success timestamp %lld %lld\n",timeStamp/90 ,ntp_time_ms);
	int ret = _decodeCompleteCallback->Decoded(decodedImage);
	if (ret != 0)
		return ret;
	return WEBRTC_VIDEO_CODEC_OK;
}
}
