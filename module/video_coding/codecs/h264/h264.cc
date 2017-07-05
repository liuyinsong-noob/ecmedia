#include <stdlib.h>
#include <string.h>

#include "h264.h"
#include "webrtc_libyuv.h"

#if defined(_WIN32)
#include <windows.h>
#include <MMSystem.h> //timeGetTime
#else
#include <sys/time.h>
#endif

#include "module_common_types.h"
#include "Trace.h"


namespace cloopenwebrtc
{  
    extern int printTime();
    
H264Encoder* H264Encoder::Create() {
    return new H264Encoder();
}

H264Encoder::H264Encoder()    : encoded_image_(),
    encoded_complete_callback_(NULL),
    inited_(false),
    picture_id_(0),
    periodicKeyFrames_(true),
    framenum_(0),
    encoder_(NULL),
    stap_a_allowed(false),
    bitrate(256000),
    fps(12),
    mode(0),
    generate_keyframe(false),
	num_of_cores_(0){
        memset(&codec_, 0, sizeof(codec_));
#ifdef HAVE_H264_BITSTREAM
		_bitStreamBeforeSend = fopen("encoderH264.bit", "wb");
#endif

}

H264Encoder::~H264Encoder() {
#ifdef HAVE_H264_BITSTREAM
	fclose(_bitStreamBeforeSend);
#endif
#ifdef HAVE_H264_LOG
	fclose (_H264LogFile);
#endif

	Release();
}

int H264Encoder::Release() {

    if (encoded_image_._buffer != NULL) {
        delete [] encoded_image_._buffer;
        encoded_image_._buffer = NULL;
    }
    if (encoder_ != NULL) {
        x264_encoder_close(encoder_);
        encoder_ = NULL;
    }
    inited_ = false;
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::SetRates(uint32_t new_bitrate_kbit, uint32_t new_framerate) {
    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (new_framerate < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    // update bit rate
    if (codec_.maxBitrate > 0 && new_bitrate_kbit > codec_.maxBitrate) {
        new_bitrate_kbit = codec_.maxBitrate;
    }
    
	if (codec_.maxFramerate >0 && new_framerate > codec_.maxFramerate){
		new_framerate = codec_.maxFramerate;
	}
    

	if(encoder_) {
		x264_param_t curparms;
		x264_encoder_parameters((x264_t*)encoder_, &curparms);
		curparms.i_fps_num = new_framerate;
		curparms.i_fps_den = 1;
		curparms.rc.i_vbv_max_bitrate = new_bitrate_kbit;
		curparms.rc.i_vbv_buffer_size = new_bitrate_kbit;

		int retval = x264_encoder_reconfig(encoder_, &curparms);
		if (retval < 0)	
			return WEBRTC_VIDEO_CODEC_ERROR;
	}
	    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::InitEncode(const VideoCodec* inst,
                           int32_t number_of_cores,
                           size_t max_payload_size)
{
	x264_param_t param_;

	codec_ = *inst;
	num_of_cores_ = number_of_cores;	

    if ((inst == NULL) || (inst->maxFramerate < 1) 
		|| (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate) 
		|| (inst->width < 1 || inst->height < 1)
		|| (number_of_cores < 1)) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    } 
    int retVal = Release();
    if (retVal < 0) {
        return retVal;
    }

	SetX264EncodeParameters(param_, inst->mode);
    encoder_ =  x264_encoder_open( &param_);
    if (!encoder_) {
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
	
	encoded_image_._size =  CalcBufferSize(cloopenwebrtc::kI420, codec_.width, codec_.height);
    encoded_image_._buffer = new uint8_t[encoded_image_._size];
    encoded_image_._length = 0;
    encoded_image_._completeFrame = false;

    // random start 16 bits is enough.
    picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;
    inited_ = true;
    return WEBRTC_VIDEO_CODEC_OK;
}
    
int H264Encoder::Encode(const I420VideoFrame& input_image,
                       const CodecSpecificInfo* codec_specific_info,
                       const std::vector<VideoFrameType>* frame_types)
{
	VideoFrameType frameType=kDeltaFrame;
	x264_picture_t xpic;
	x264_picture_t oxpic;
	x264_nal_t *xnals=NULL;
	int num_nals=0;
	bool send_key_frame = false;

    if (!inited_
		|| (encoded_complete_callback_ == NULL) ) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (input_image.IsZeroSize()) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
	
	for (size_t i = 0; i < frame_types->size();++i) {
		if ((*frame_types)[i] == kKeyFrame) {
			send_key_frame = true;
			frameType = kKeyFrame;
			break;
		}
	}

	if(codec_.width != input_image.width() || codec_.height != input_image.height())
	{
        Release();
//		x264_param_t curparms;
//		x264_encoder_parameters(encoder_, &curparms);
//		curparms.i_width = input_image.width();
//		curparms.i_height = input_image.height();
//		x264_encoder_reconfig(encoder_, &curparms);
		codec_.width = input_image.width();
		codec_.height = input_image.height();
		frameType = kKeyFrame;
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"x264_encoder_reconfig:_framewidth=%d _frameheight=%d", codec_.width, codec_.height);
        InitEncode(&codec_, num_of_cores_, 30000);
        
	}

	InitializeX264Pic(input_image, xpic, oxpic, frameType);
  
    picture_id_ = (framenum_ + 1) & 0x7FFF;  // prepare next
    if (framenum_ == 0) {
        printTime();printf("seansean h264 encode first frame encoded 111111\n");
    }
    
    int ret = x264_encoder_encode(encoder_,&xnals,&num_nals,&xpic,&oxpic);

	if (ret > 0)
	{
		
		CodecSpecificInfo codec;
		CodecSpecificInfoH264 *h264Info = &(codec.codecSpecific.H264);
		RTPFragmentationHeader fragment;
		bool bNonReference = CopyEncodedImage(fragment, xnals, num_nals, &oxpic, input_image, codec_.mode);
		codec.codecType = kVideoCodecH264;
		h264Info->pictureId = framenum_;
		h264Info->nonReference = bNonReference;
		encoded_complete_callback_->Encoded(encoded_image_, &codec, &fragment);
        if (framenum_ == 0) {
            printTime();printf("seansean h264 encode first frame encoded 111112\n");
        }
        
		framenum_++;
	}
	else{
		WEBRTC_TRACE(cloopenwebrtc::kTraceError,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"x264_encoder_encode() error=%d.", ret);
	}

    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::SetChannelParameters(uint32_t /*packet_loss*/, int64_t rtt) {

    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::RegisterEncodeCompleteCallback(
                                               EncodedImageCallback* callback) {
    encoded_complete_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32 H264Encoder::SetPeriodicKeyFrames(bool enable)
{
    periodicKeyFrames_ = enable;
    return WEBRTC_VIDEO_CODEC_OK;
}

WebRtc_Word32 H264Encoder::CodecConfigParameters(WebRtc_UWord8* /*buffer*/, WebRtc_Word32 /*size*/)
{
    return WEBRTC_VIDEO_CODEC_OK;
}


void H264Encoder::SetX264EncodeParameters(x264_param_t &params, VideoCodecMode mode)
{
	x264_param_t *p_params = &params;
	if (mode==kRealtimeVideo || mode == kSaveToFile)
	{
		x264_param_default_preset(p_params,x264_preset_names[2],"zerolatency");

	}else if (mode == kScreensharing)
	{
		x264_param_default_preset(p_params,x264_preset_names[2],"stillimage");
	}
	x264_param_apply_profile(p_params, x264_profile_names[0]);

	p_params->i_level_idc = 40;  //编码复杂度
	p_params->i_width=codec_.width;
	p_params->i_height=codec_.height;
	p_params->i_fps_num = codec_.maxFramerate;
	p_params->i_fps_den=1;
	p_params->i_slice_max_size=1300;
	p_params->b_annexb=1; //already set by defaule:默认支持字节流格式，即包含nal起始码前缀0x00 00 00 01；
	//p_params->b_intra_refresh = true;
	p_params->b_repeat_headers;
	p_params->i_keyint_max = 50;

	p_params->rc.i_vbv_max_bitrate = codec_.startBitrate;
	p_params->rc.i_vbv_buffer_size = codec_.startBitrate;
}

void H264Encoder::InitializeX264Pic(const I420VideoFrame& input_image, x264_picture_t &xpic, x264_picture_t &oxpic, VideoFrameType frame_type)
{
	memset(&xpic, 0, sizeof(xpic));
	memset(&oxpic, 0, sizeof(oxpic));

	x264_picture_init(&xpic);

	xpic.i_type=X264_TYPE_AUTO;
	if(frame_type == kKeyFrame) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceApiCall,
			cloopenwebrtc::kTraceVideoCoding,
			0,
			"x264_encoder_intra_refresh called.");
		xpic.i_type = X264_TYPE_IDR;
		x264_encoder_intra_refresh(encoder_);
	}

	xpic.i_qpplus1=0;
	xpic.i_pts=framenum_;
	xpic.param=NULL;
	xpic.img.i_csp=X264_CSP_I420;

	xpic.img.i_plane= 3;
	xpic.img.plane[0] = const_cast<uint8_t*>(input_image.buffer(kYPlane));
	xpic.img.plane[1] = const_cast<uint8_t*>(input_image.buffer(kUPlane));
	xpic.img.plane[2] = const_cast<uint8_t*>(input_image.buffer(kVPlane));
	xpic.img.plane[3] = 0;
	xpic.img.i_stride[0] = input_image.stride(kYPlane);
	xpic.img.i_stride[1] = input_image.stride(kUPlane);
	xpic.img.i_stride[2] = input_image.stride(kVPlane);
	xpic.img.i_stride[3] = 0;
}

bool H264Encoder::CopyEncodedImage(RTPFragmentationHeader &fragment, void *xnals, int num_nals, void *oxpic, const I420VideoFrame &input_image, VideoCodecMode mode)
{
#define NALU_START_PREFIX_LENGTH 4
	assert(num_nals>0);
	assert(xnals != NULL);
	x264_nal_t *_xnals		= (x264_nal_t*)xnals;
	x264_picture_t *_oxpic	= (x264_picture_t *)oxpic;
	fragment.VerifyAndAllocateFragmentationHeader(num_nals);

	encoded_image_._encodedHeight	= codec_.height;
	encoded_image_._encodedWidth	= codec_.width;
	encoded_image_._frameType		= _oxpic->b_keyframe ? kKeyFrame : kDeltaFrame;
	encoded_image_._timeStamp		= input_image.timestamp();
	encoded_image_._length          = 0;
	encoded_image_._completeFrame   = true;

	for (int i=0; i<num_nals; i++)
	{
		x264_nal_t *current_nal = &_xnals[i];
		int offset = 0;
		if( mode != kSaveToFile ) {
			bool b_long_startcode = current_nal->b_long_startcode;
			offset = b_long_startcode ? NALU_START_PREFIX_LENGTH : (NALU_START_PREFIX_LENGTH-1);
		}
		memcpy(encoded_image_._buffer+encoded_image_._length, current_nal->p_payload+offset, current_nal->i_payload-offset);
		fragment.fragmentationLength[i] = current_nal->i_payload-offset;
		fragment.fragmentationOffset[i] = encoded_image_._length;
		fragment.fragmentationPlType[i] = current_nal->i_type;
		encoded_image_._length += current_nal->i_payload - offset;
	}
#ifdef HAVE_H264_BITSTREAM
	for (int i=0; i<num_nals; i++)
	{
		x264_nal_t *current_nal = &_xnals[i];
		fwrite(current_nal->p_payload, 1, current_nal->i_payload, _bitStreamBeforeSend);
	}
#endif

	return (_oxpic->b_keyframe==kKeyFrame) ? false : true;
}


} //namespace cloopenwebrtc
