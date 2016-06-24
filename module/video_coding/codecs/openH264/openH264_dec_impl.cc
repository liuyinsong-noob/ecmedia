/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 * This file contains the WEBRTC H264 wrapper implementation
 *
 */

#include "openH264.h"
#include "openH264_dec_impl.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

//#include "libyuv.h"
#include "module_common_types.h"
#include "trace.h"
#include "tick_util.h"
#include "bitstream_parser.h"


//#define DEBUG_DECODED_YUV
//#define DEBUG_FRAGMENT_FILE
//#define DEBUG_ENCODER_264
//#define DEBUG_DISTRACTE_FILE


#define RATIO_4_3
#define NUM_SPATIAL_LAYER  3  //range 0~4
#define NUM_TEMPORAL_LAYER 2  //range 1~4

//#define EXTRACT_BS_LAYER
#define EXTRACT_LAYER_TID  2  //range 0~3
#define EXTRACT_LAYER_DID  2  //range 0~3


#define ENABLE_ENCODE_SVC
#define ENABLE_DECODE_SVC

#define EPSN (0.000001f) // (1e-6) // desired float precision
#define WELS_MAX(x, y) ((x) > (y) ? (x) : (y))
#define WELS_ROUND(x) ((int32_t)(0.5+(x)))

namespace cloopenwebrtc {

OpenH264Decoder* OpenH264Decoder::Create() {
  return new OpenH264DecoderImpl();
}

OpenH264DecoderImpl::OpenH264DecoderImpl()
    : decode_complete_callback_(NULL),
      inited_(false),
      decoder_(NULL),
	  number_of_cores_(0){
  memset(&codec_, 0, sizeof(codec_));
#ifdef DEBUG_DECODED_YUV
  	_decodedYUV = fopen("decoded.yuv", "wb");
#endif

#ifdef DEBUG_FRAGMENT_FILE
	_fragFIle = fopen("frag.264","wb");
#endif

#ifdef DEBUG_DISTRACTE_FILE
	_distractorFile = fopen("distract.264","wb");
#endif
}

OpenH264DecoderImpl::~OpenH264DecoderImpl() { 
  Release();
 
#ifdef DEBUG_DECODED_YUV
  fclose(_decodedYUV);
#endif

#ifdef DEBUG_FRAGMENT_FILE
  fclose(_fragFIle);
#endif

#ifdef DEBUG_DISTRACTE_FILE
  fclose(_distractorFile);
#endif
}

int OpenH264DecoderImpl::Reset() {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
 // InitDecode(&codec_, number_of_cores_);
  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264DecoderImpl::InitDecode(const VideoCodec* inst, int number_of_cores) {
  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  int ret_val = Release();
  if (ret_val < 0) {
    return ret_val;
  }
  if (decoder_ == NULL) {
    ret_val = WelsCreateDecoder(&decoder_);
    if (ret_val != 0) {
      decoder_ = NULL;
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }
  number_of_cores_ = number_of_cores;

  uiTimeStamp = 0;
  SDecodingParam dec_param;
  memset(&dec_param, 0, sizeof(SDecodingParam));
  dec_param.eOutputColorFormat = videoFormatI420;
  dec_param.uiTargetDqLayer = UCHAR_MAX-1;
  dec_param.eEcActiveIdc = ERROR_CON_SLICE_COPY_CROSS_IDR;
  dec_param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
 
  ret_val = decoder_->Initialize(&dec_param);
  if (ret_val != 0) {
    decoder_->Uninitialize();
    WelsDestroyDecoder(decoder_);
    decoder_ = NULL;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (&codec_ != inst) {
    // Save VideoCodec instance for later; mainly for duplicating the decoder.
    codec_ = *inst;
  }

  inited_ = true;

  // Always start with a complete key frame.
   WEBRTC_TRACE(cloopenwebrtc::kTraceApiCall, cloopenwebrtc::kTraceVideoCoding, -1,
               "OpenH264DecoderImpl::InitDecode(width:%d, height:%d, framerate:%d, start_bitrate:%d, max_bitrate:%d)",
               inst->width, inst->height, inst->maxFramerate, inst->startBitrate, inst->maxBitrate);
  return WEBRTC_VIDEO_CODEC_OK;
}


#define TemporalID 0
#define DependencyID 0

int OpenH264DecoderImpl::Decode(const EncodedImage& input_image,
							bool missing_frames,
							const RTPFragmentationHeader* fragmentation,
							const CodecSpecificInfo* codec_specific_info,
							int64_t /*render_time_ms*/)
{
	return -1;
#if 0
	if (!inited_) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
			"H264DecoderImpl::Decode, decoder is not initialized");
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}

	if (decode_complete_callback_ == NULL) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
			"H264DecoderImpl::Decode, decode complete call back is not set");
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}

	if (input_image._buffer == NULL) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
			"H264DecoderImpl::Decode, null buffer");
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (!codec_specific_info) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
			"H264EncoderImpl::Decode, no codec info");
		return WEBRTC_VIDEO_CODEC_ERROR;
	}
	if (codec_specific_info->codecType != kVideoCodecH264) {
		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
			"H264EncoderImpl::Decode, non h264 codec %d", codec_specific_info->codecType);
		return WEBRTC_VIDEO_CODEC_ERROR;
	}

#ifdef DEBUG_DECODED_YUV
	{
		fwrite(input_image._buffer, sizeof(uint8_t), 
			input_image._length, _decodedYUV);
	}
#endif
	int num = 0;
#ifdef DEBUG_FRAGMENT_FILE
	int i=0;
	while (num < fragmentation->fragmentationVectorSize)
	{
		int offset = fragmentation->fragmentationOffset[num];
		int len = fragmentation->fragmentationLength[num];
		
			fwrite(input_image._buffer+offset,sizeof(uint8_t), len, _fragFIle);
		
		num++;
	}
#endif

	uint8_t *data[3];
	SBufferInfo buffer_info;
	memset(data, 0, sizeof(data));
	memset(&buffer_info, 0, sizeof(SBufferInfo));

#ifdef EXTRACT_BS_LAYER
	//如果接收的未经分离的流，需分离后判断DID、TID
	num=fragmentation->fragmentationVectorSize;
	int Tid = EXTRACT_LAYER_TID;
	int Did = EXTRACT_LAYER_DID;

	bool decode_nal_after_prefix = false;
	for(int i=0; i<num; i++)
	{
		int offset = fragmentation->fragmentationOffset[i];
		int length = fragmentation->fragmentationLength[i];
		bool isPrefixNal = false;
		bool NeedDecode = PartitionNeedDecode(input_image._buffer+offset, length, Tid, Did, isPrefixNal);

		if (NeedDecode && isPrefixNal)
		{
			decode_nal_after_prefix = true;
		}

		if (NeedDecode)
		{
#ifdef DEBUG_DISTRACTE_FILE
			fwrite(input_image._buffer+offset, sizeof(uint8_t), length, _distractorFile);
#endif
			decoder_->DecodeFrameNoDelay(input_image._buffer+offset, length, data, &buffer_info);
			if (buffer_info.iBufferStatus == 1)
			{
				break;
			}	
		}else if (decode_nal_after_prefix)
		{
#ifdef DEBUG_DISTRACTE_FILE
			fwrite(input_image._buffer+offset, sizeof(uint8_t), length, _distractorFile);
#endif
			decoder_->DecodeFrameNoDelay(input_image._buffer+offset, length, data, &buffer_info);
			if (buffer_info.iBufferStatus == 1)
			{
				break;
			}	
			decode_nal_after_prefix = false;
		}	
	}
#else
	//如果接收的是已经分离的流，可直接解码
	{
		num = fragmentation->fragmentationVectorSize;
		for(int i=0; i< num; i++)
		{
			int offset = fragmentation->fragmentationOffset[i];
			int length = fragmentation->fragmentationLength[i];
			
			decoder_->DecodeFrameNoDelay(input_image._buffer+offset, length, data, &buffer_info);
			if (buffer_info.iBufferStatus == 1)
			{
				break;
			}
		}	
	}
#endif
	if (buffer_info.iBufferStatus == 1)
	{
		int iwidth = buffer_info.UsrData.sSystemBuffer.iWidth;
		int iheight = buffer_info.UsrData.sSystemBuffer.iHeight;
		int newSize = (iwidth*iheight*3)>>1;

		if (decoded_image_._buffer==NULL || decoded_image_._size<newSize)
		{
			if (decoded_image_._buffer != NULL)
			{
				free(decoded_image_._buffer);
				decoded_image_._buffer = NULL;
			}
			uint8_t *newbuff = (uint8_t*)malloc(newSize);
			if (newbuff == NULL)
			{
				return WEBRTC_VIDEO_CODEC_MEMORY;
			}	
			decoded_image_._buffer = newbuff;
			decoded_image_._size = newSize;		
		}

		int retval = PrepareRawImage(data, &buffer_info);
		if (retval == WEBRTC_VIDEO_CODEC_OK)
		{
			decoded_image_._timeStamp = input_image._timeStamp;
			decode_complete_callback_->Decoded(decoded_image_);
			return retval;
		}
	}	
	return WEBRTC_VIDEO_CODEC_ERROR;
#endif
}


int OpenH264DecoderImpl::RegisterDecodeCompleteCallback(
    DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264DecoderImpl::Release() {
 
	return -1;
#if 0 
  if (decoder_ != NULL) {
    decoder_->Uninitialize();
    WelsDestroyDecoder(decoder_);
    decoder_ = NULL;
  }
  if (decoded_image_._buffer != NULL)
  {
	  delete [] decoded_image_._buffer;
	  decoded_image_._buffer = NULL;
  }
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
#endif
}

VideoDecoder* OpenH264DecoderImpl::Copy() {
	return NULL;
#if 0
  // Sanity checks.
  if (!inited_) {
    // Not initialized.
    assert(false);
    return NULL;
  }
  if (decoded_image_._buffer == NULL) {
    // Nothing has been decoded before; cannot clone.
    return NULL;
  }
 
  // Create a new VideoDecoder object
  OpenH264DecoderImpl *copy = new OpenH264DecoderImpl;

  // Initialize the new decoder
  if (copy->InitDecode(&codec_, 1) != WEBRTC_VIDEO_CODEC_OK) {
    delete copy;
    return NULL;
  }

  return static_cast<VideoDecoder*>(copy);
#endif
}


bool OpenH264DecoderImpl::PartitionNeedDecode(uint8_t* nalu_with_start_code, int length, int Tid, int Did, bool &prefix_nalu)
{
#define NALU_START_PREFIX_LEN	4
#define NALU_HEADER_LEN			1
	bool retval = false;
	uint8_t nalu_header = nalu_with_start_code[NALU_START_PREFIX_LEN];
	int nalu_type = nalu_header & 0x1F;

	uint32_t pps_id = 0;
	uint32_t currTid = 0;
	uint32_t currDid = 0;
	uint32_t currQid = 0;

	BitstreamParser bsParser(nalu_with_start_code+NALU_START_PREFIX_LEN+NALU_HEADER_LEN, length-NALU_HEADER_LEN-NALU_START_PREFIX_LEN);	

	if (nalu_type == SVCNAL_PPS)
	{
		uint8_t *bitstream;
		bitstream = nalu_with_start_code+5;
		pps_id = bsParser.GetUE();	
		pps_id %= NUM_SPATIAL_LAYER;	
	}
	if (nalu_type==SVCNAL_PREFIX || nalu_type==SVCNAL_ENHANCEMENT)
	{
		uint8_t nalu_byte1 = nalu_with_start_code[NALU_START_PREFIX_LEN+NALU_HEADER_LEN];
		uint8_t nalu_byte2 = nalu_with_start_code[NALU_START_PREFIX_LEN+NALU_HEADER_LEN+1];
		uint8_t nalu_byte3 = nalu_with_start_code[NALU_START_PREFIX_LEN+NALU_HEADER_LEN+2];
		uint8_t svc_flag = (nalu_byte1 & 0x80)>>7;
		currDid = (nalu_byte2 & 0x70)>>4; // *xxx ****
		currQid = (nalu_byte2 & 0x0F);     //**** xxxx
		currTid = (nalu_byte3 & 0xE0)>>5; // xxx* ****
	}

	switch(nalu_type)
	{
	case  SVCNAL_SPS:
	case  SVCNAL_SUBSET_SPS:
		retval = true;
		break;
	case  SVCNAL_PPS:
		if (pps_id == Did)
		{
			retval = true;
		}
		break;
	case SVCNAL_PREFIX:
		if (currDid <= Did && currTid <= Tid)
		{
			retval = true;
		}
		prefix_nalu = true;
		break;
	case  SVCNAL_ENHANCEMENT:
		if (currDid <= Did && currTid <= Tid)
		{
			retval = true;
		}
		break;
	default:
		break;
	}
	return retval;
}

int OpenH264DecoderImpl::PrepareRawImage(uint8_t** data, SBufferInfo *info)
{
	return -1;
#if 0
	int iwidth = info->UsrData.sSystemBuffer.iWidth;
	int iheight = info->UsrData.sSystemBuffer.iHeight;
	
	
	//copy Y/U/V line by line
	uint8_t* dstBuffer;
	uint8_t* srcBuffer;
	int offset = 0;
	int i=0;

	dstBuffer = decoded_image_.GetPlane(kYPlane);
	srcBuffer = data[0];
	for (i=0; i< iheight; i++)
	{
		memcpy(dstBuffer+offset, srcBuffer, iwidth);
		offset += iwidth;
		srcBuffer += info->UsrData.sSystemBuffer.iStride[0];
	}

	dstBuffer = decoded_image_.GetPlane(kUPlane);
	srcBuffer = data[1];
	for (i=0; i<iheight/2; i++)
	{
		memcpy(dstBuffer+offset, srcBuffer, iwidth/2);
		offset += iwidth/2;
		srcBuffer += info->UsrData.sSystemBuffer.iStride[1];
	}

	dstBuffer = decoded_image_.GetPlane(kVPlane);
	srcBuffer = data[2];
	for (i=0; i<iheight/2; i++)
	{
		memcpy(dstBuffer+offset, srcBuffer, iwidth/2);
		offset += iwidth/2;
		srcBuffer += info->UsrData.sSystemBuffer.iStride[1];
	}
	

	/*decoded_image_.width() = info->UsrData.sSystemBuffer.iWidth;
	decoded_image_.height() = info->UsrData.sSystemBuffer.iHeight;
	decoded_image_._length = offset;*/

	decoded_image_.set_width(info->UsrData.sSystemBuffer.iWidth);
	decoded_image_.set_height(info->UsrData.sSystemBuffer.iHeight);
	//decoded_image_.ResetSize()
	return WEBRTC_VIDEO_CODEC_OK;
#endif
}
}  // namespace webrtc
