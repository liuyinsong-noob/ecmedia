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
#include "openH264_enc_impl.h"
#include "svc_config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#include "module_common_types.h"
#include "../system_wrappers/include/trace.h"
#include "../system_wrappers/include/tick_util.h"
#include "bitstream_parser.h"

//#define DEBUG_DECODED_YUV
//#define DEBUG_FRAGMENT_FILE
#define DEBUG_ENCODER_264
//#define DEBUG_DISTRACTE_FILE

#define NALU_START_PREFIX_LENTH 4
#define WELS_MAX(x, y) ((x) > (y) ? (x) : (y))
#define WELS_ROUND(x) ((int32_t)(0.5+(x)))

namespace cloopenwebrtc {

OpenH264Encoder* OpenH264Encoder::Create() {

  return new OpenH264EncoderImpl();
}

OpenH264EncoderImpl::OpenH264EncoderImpl()
    : encoded_image_(),
      encoded_complete_callback_(NULL),
      inited_(false),
      first_frame_encoded_(false),
      timestamp_(0),
      encoder_(NULL),
	  framerate_(0.0f),
	  frameIndx_(0),
	  number_of_cores_(0){
  memset(&codec_, 0, sizeof(codec_));
  memset(&bitrateInfo_, 0, sizeof(bitrateInfo_));
  uint32_t seed = static_cast<uint32_t>(TickTime::MillisecondTimestamp());
  srand(seed);
#ifdef DEBUG_ENCODER_264
  _encodedFile = fopen("encoded.264", "wb");
#endif
}

OpenH264EncoderImpl::~OpenH264EncoderImpl() {
#ifdef DEBUG_ENCODER_264
	fclose(_encodedFile);
#endif
  Release();
}

int OpenH264EncoderImpl::Release() {
  if (encoded_image_._buffer != NULL) {
    delete [] encoded_image_._buffer;
    encoded_image_._buffer = NULL;
  }
  if (encoder_ != NULL) {
    WelsDestroySVCEncoder(encoder_);
    encoder_ = NULL;
  }
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264EncoderImpl::SetRates(uint32_t new_bitrate_kbit,
                             uint32_t new_framerate) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (codec_.codecSpecific.H264Svc.numberOfSpatialLayers > 1 
	  || codec_.codecSpecific.H264Svc.numberOfTemporalLayers > 1)
  {
	  return WEBRTC_VIDEO_CODEC_OK; //非点对点，无需调节自适应帧率和码率
  }
  
  if (new_framerate < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  // update bit rate
  if (codec_.maxBitrate > 0 && new_bitrate_kbit > codec_.maxBitrate) {
    new_bitrate_kbit = codec_.maxBitrate;
  }
  bitrateInfo_.iLayer = SPATIAL_LAYER_0;
  bitrateInfo_.iBitrate = new_bitrate_kbit*1000;
  framerate_ = (float)new_framerate;

  WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
	  "OpenH264EncoderImpl::SetRates, new_bitrate_kbit=%u, new_framerate=%d",
	  new_bitrate_kbit, new_framerate);

  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264EncoderImpl::InitEncode(const VideoCodec* inst,
                               int number_of_cores,
                               size_t /*max_payload_size*/) {
  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->maxFramerate < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  // allow zero to represent an unspecified maxBitRate
  if (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->width < 1 || inst->height < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (number_of_cores < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  number_of_cores_ = number_of_cores;
  int ret_val= Release();
  if (ret_val < 0) {
    return ret_val;
  }
  if (encoder_ == NULL) {
	ret_val = WelsCreateSVCEncoder(&encoder_);
    if (ret_val != 0) {
     WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
  	              "H264EncoderImpl::InitEncode() fails to create encoder ret_val %d",
    	           ret_val);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }

  SetSVCEncoderParameters(inst, sSvcParam);
  if (inst->codecSpecific.H264Svc.numberOfSpatialLayers==1 
	  && inst->codecSpecific.H264Svc.numberOfTemporalLayers==1)
  {
	  bitrateInfo_.iLayer = SPATIAL_LAYER_0;
	  bitrateInfo_.iBitrate = inst->startBitrate*1000;
	  framerate_ = inst->maxFramerate;
  }
  
 

  ret_val != encoder_->InitializeExt (&sSvcParam);

  if (ret_val != 0) {
	  WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
		  "H264EncoderImpl::InitEncode() fails to initialize encoder ret_val %d",
		  ret_val);
	  WelsDestroySVCEncoder(encoder_);
	  encoder_ = NULL;
	  return WEBRTC_VIDEO_CODEC_ERROR;
  }
  
  if (&codec_ != inst) {
    codec_ = *inst;
  }

  encoded_image_._size = CalcBufferSize(cloopenwebrtc::kI420, codec_.width, codec_.height);
  encoded_image_._buffer = new uint8_t[encoded_image_._size];
  encoded_image_._completeFrame = true;
  inited_ = true;
  WEBRTC_TRACE(cloopenwebrtc::kTraceApiCall, cloopenwebrtc::kTraceVideoCoding, -1,
               "OpenH264EncoderImpl::InitEncode(width:%d, height:%d, framerate:%d, temporal num:%d, spatial num:%d)",
               inst->width, inst->height, inst->maxFramerate, 
			   inst->codecSpecific.H264Svc.numberOfTemporalLayers, 
			   inst->codecSpecific.H264Svc.numberOfSpatialLayers);

  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264EncoderImpl::Encode(const I420VideoFrame& input_image,
							const CodecSpecificInfo* codec_specific_info,
							const std::vector<VideoFrameType>* frame_types) {
	if (!inited_) {
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}
	if (input_image.IsZeroSize()) {
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (encoded_complete_callback_ == NULL) {
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}
//	bool send_keyframe = (frame_types == kKeyFrame);
	bool send_keyframe = false;
	if (send_keyframe) {
		encoder_->ForceIntraFrame(true);
		WEBRTC_TRACE(cloopenwebrtc::kTraceApiCall, cloopenwebrtc::kTraceVideoCoding, -1,
			"OpenH264EncoderImpl::ForceIntraFrame(width:%d, height:%d)",
			input_image.width(), input_image.height());
	}
	// Check for change in frame size.
	if (input_image.width() != codec_.width ||
		input_image.height() != codec_.height) {
			int ret = UpdateCodecFrameSize(input_image);
			if (ret < 0) {
				return ret;
			}
	}

	SFrameBSInfo frameBsInfo;
	SSourcePicture sourcePic;
	InitializeSFBsInfo(frameBsInfo);
	InitializeSSPic(input_image, sourcePic);

	if (codec_.codecSpecific.H264Svc.numberOfSpatialLayers==1 
		&& codec_.codecSpecific.H264Svc.numberOfTemporalLayers==1)
	{
		encoder_->SetOption(ENCODER_OPTION_BITRATE, &bitrateInfo_);
		encoder_->SetOption(ENCODER_OPTION_FRAME_RATE, &framerate_);

		WEBRTC_TRACE(cloopenwebrtc::kTraceError, cloopenwebrtc::kTraceVideoCoding, -1,
			"OpenH264EncoderImpl::SetOption, new_bitrate_bit=%d, new_framerate=%f",
			bitrateInfo_.iBitrate, framerate_);
	}

	int retVal = encoder_->EncodeFrame(&sourcePic, &frameBsInfo);
	if (retVal == videoFrameTypeSkip) {
		return WEBRTC_VIDEO_CODEC_OK;
	}



	RTPFragmentationHeader fragment;
	bool bNonReference = CopyEncodedImage(fragment, frameBsInfo, input_image);

	CodecSpecificInfo codec;
	CodecSpecificInfoH264SVC *h264SvcInfo = &(codec.codecSpecific.H264SVC);
	codec.codecType = kVideoCodecH264SVC;
	h264SvcInfo->pictureId = frameBsInfo.iTemporalId;
	h264SvcInfo->last = 1;
	h264SvcInfo->nonReference = bNonReference;

	encoded_complete_callback_->Encoded(encoded_image_, &codec, &fragment);
	frameIndx_++;

	if (!first_frame_encoded_) {
		first_frame_encoded_ = true;
	}

	return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264EncoderImpl::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  encoded_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264EncoderImpl::SetChannelParameters(uint32_t /*packet_loss*/, int64_t rtt) {
  // ffs
  return WEBRTC_VIDEO_CODEC_OK;
}

int OpenH264EncoderImpl::UpdateCodecFrameSize(const I420VideoFrame& input_image) {
  codec_.width = input_image.width();
  codec_.height = input_image.height();
  // ffs
  return WEBRTC_VIDEO_CODEC_OK;
}

void OpenH264EncoderImpl::SetSVCEncoderParameters(const VideoCodec *inst, SEncParamExt &sSvcParam)
{
    double video_ratio = 0.0f;
	int width = inst->width;
	int height = inst->height;

	video_ratio = (double)width/(double)height;
	encoder_->GetDefaultParams(&sSvcParam);

	if (video_ratio < 1.4f)
	{
		FillSpecificParameters_Ratio_4to3(sSvcParam);
	}else 
	{
		FillSpecificParameters_Ratio_16to9(sSvcParam);
	}
	
	sSvcParam.iTemporalLayerNum = inst->codecSpecific.H264Svc.numberOfTemporalLayers;
	sSvcParam.iSpatialLayerNum	= inst->codecSpecific.H264Svc.numberOfSpatialLayers;
	sSvcParam.fMaxFrameRate		= inst->maxFramerate;
	sSvcParam.iPicWidth			= inst->width;
	sSvcParam.iPicHeight		= inst->height;
	sSvcParam.iUsageType		= CAMERA_VIDEO_REAL_TIME;
	sSvcParam.eSpsPpsIdStrategy = INCREASING_ID;
	sSvcParam.iComplexityMode	= MEDIUM_COMPLEXITY;
	sSvcParam.uiIntraPeriod		= 320;
	sSvcParam.bEnableAdaptiveQuant		= true;
	sSvcParam.bEnableSceneChangeDetect	= true;
	sSvcParam.bEnableDenoise			= false;
	sSvcParam.bEnableBackgroundDetection= true;
	sSvcParam.bEnableFrameCroppingFlag	= false;
	sSvcParam.bEnableFrameSkip			= false;
	sSvcParam.bEnableSSEI				= true;
	sSvcParam.bEnableLongTermReference	= false;
	sSvcParam.bPrefixNalAddingCtrl		= false;
	sSvcParam.iRCMode					= RC_BITRATE_MODE;
	sSvcParam.iTargetBitrate			= 2500*1000;
	sSvcParam.iMaxBitrate				= 2500*1000;

	//sSvcParam.uiMaxNalSize				= 1300;
	sSvcParam.iMultipleThreadIdc		= number_of_cores_*2;
	
	for (int iLayer = 0; iLayer < sSvcParam.iSpatialLayerNum; iLayer++) {
		SSpatialLayerConfig* pDLayer = &sSvcParam.sSpatialLayers[iLayer];
		sSvcParam.iPicWidth = WELS_MAX (sSvcParam.iPicWidth, pDLayer->iVideoWidth);
		sSvcParam.iPicHeight = WELS_MAX (sSvcParam.iPicHeight, pDLayer->iVideoHeight);
	}

	if (sSvcParam.iTemporalLayerNum==1 && sSvcParam.iSpatialLayerNum==1)
	{
		SSpatialLayerConfig* pDLayer = &sSvcParam.sSpatialLayers[0];
		pDLayer->iVideoWidth = inst->width;
		pDLayer->iVideoHeight = inst->height;
		pDLayer->fFrameRate = inst->maxFramerate;
		pDLayer->iSpatialBitrate = inst->startBitrate*1000;
	}
	
}
void OpenH264EncoderImpl::InitializeSFBsInfo(SFrameBSInfo &info)
{
	memset(&info, 0, sizeof(SFrameBSInfo));
}
void OpenH264EncoderImpl::InitializeSSPic(const I420VideoFrame& input_image,SSourcePicture &pic)
{
	memset(&pic,0,sizeof(SSourcePicture));
	pic.iPicWidth = input_image.width();
	pic.iPicHeight = input_image.height();
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = pic.iPicWidth;
	pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;

//	pic.pData[0] = (uint8_t*)(input_image._buffer);
	pic.pData[1] = (uint8_t*)(pic.pData[0] + input_image.width()*input_image.height());
	pic.pData[2] = (uint8_t*)(pic.pData[1] + (input_image.width()*input_image.height()>> 2));
	pic.uiTimeStamp = WELS_ROUND (frameIndx_ * (1000 / sSvcParam.fMaxFrameRate));
}

bool OpenH264EncoderImpl::CopyEncodedImage(RTPFragmentationHeader &fragment, SFrameBSInfo &frameBsInfo, const I420VideoFrame &input_image)
{
	int layer = 0;
	uint32_t totalNaluCount = 0;
	uint32_t totalNaluIndex = 0;
	bool bNonReference = true;

	while (layer < frameBsInfo.iLayerNum) {
		const SLayerBSInfo* layer_bs_info = &frameBsInfo.sLayerInfo[layer];
		totalNaluCount += layer_bs_info->iNalCount;
		layer++;
	}

	fragment.VerifyAndAllocateFragmentationHeader(totalNaluCount);
	

	encoded_image_._encodedWidth	= codec_.width;
	encoded_image_._encodedHeight	= codec_.height;
	encoded_image_._frameType       = (frameBsInfo.eFrameType==videoFrameTypeIDR || frameBsInfo.eFrameType==videoFrameTypeI) ? kKeyFrame : kDeltaFrame;
	encoded_image_._timeStamp       = input_image.timestamp();
	encoded_image_._length          = 0;
	encoded_image_._completeFrame   = true;

	layer = 0;
	while (layer < frameBsInfo.iLayerNum) {
		const SLayerBSInfo* layer_bs_info = &frameBsInfo.sLayerInfo[layer];
		if (layer_bs_info != NULL) {
			int layer_size = 0;
			int nalu_begin  = NALU_START_PREFIX_LENTH;
			uint8_t* nalu_buffer = NULL;
			char nalu_type = 0;
			bool isPrefixNal = false;

			for (int nalu_index = 0; nalu_index < layer_bs_info->iNalCount; nalu_index++) {
				uint32_t currentNaluSize = layer_bs_info->pNalLengthInByte[nalu_index] - NALU_START_PREFIX_LENTH;
				nalu_buffer  = layer_bs_info->pBsBuf + nalu_begin;
				nalu_type    = (nalu_buffer[0] & 0x1F);
				if (nalu_type == 7 || nalu_type== 8 || nalu_type==5)
				{
					bNonReference = false;
				}
				memcpy(encoded_image_._buffer + encoded_image_._length, nalu_buffer, currentNaluSize);

				encoded_image_._length          += currentNaluSize;
				fragment.fragmentationOffset[totalNaluIndex] = encoded_image_._length - currentNaluSize;
				fragment.fragmentationLength[totalNaluIndex] = currentNaluSize;
				fragment.fragmentationPlType[totalNaluIndex] = layer_bs_info->uiLayerType;
				fragment.fragmentationTimeDiff[totalNaluIndex] = 0;
		
				fragment.SvcFrameInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(frameBsInfo.eFrameType&0x03)<<6;
				fragment.SvcFrameInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(frameBsInfo.iLayerNum&0x07)<<3;
				fragment.SvcFrameInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(layer_bs_info->uiLayerType&0x1)<<2;
				fragment.SvcFrameInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(layer_bs_info->uiQualityId&0x3);

				fragment.SvcLayerInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(layer_bs_info->iNalCount&0x0f)<<4;
				fragment.SvcLayerInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(layer_bs_info->uiSpatialId&0x03)<<2; 
				fragment.SvcLayerInfo[totalNaluIndex] += static_cast<WebRtc_UWord8>(layer_bs_info->uiTemporalId&0x03);

				fragment.SvcFrameNum[totalNaluIndex] = frameIndx_;
				
				totalNaluIndex++;
				layer_size  = layer_bs_info->pNalLengthInByte[nalu_index];
				nalu_begin  += layer_size;
			}
#ifdef DEBUG_ENCODER_264
			int offset=0;
			for (int nal_index = 0; nal_index < layer_bs_info->iNalCount; nal_index++)
			{
				fwrite(layer_bs_info->pBsBuf+offset, 1, layer_bs_info->pNalLengthInByte[nal_index], _encodedFile);
				offset+=layer_bs_info->pNalLengthInByte[nal_index];
			}			
#endif
		}
		layer++;
	}
	return bNonReference ? true : false;
}

void OpenH264EncoderImpl::ParseFragmentSVCLayer(RTPFragmentationHeader &fragment, uint8_t layer_index, uint8_t *nalu, int length, bool &prefix_nalu)
{
#define NALU_HEADER_LEN			1
	uint8_t nalu_header = nalu[0];
	int nalu_type = nalu_header & 0x1F;
	uint32_t	layInfo= nalu_header << 24;

	uint8_t pps_id = 0;
	uint8_t currTid = 0;
	uint8_t currDid = 0;
	uint8_t currQid = 0;

	BitstreamParser bsParser(nalu+NALU_HEADER_LEN, length);	

	if (nalu_type == SVCNAL_PPS)
	{
		pps_id = bsParser.GetUE();	
		pps_id %= sSvcParam.iSpatialLayerNum;	
	}

	if (nalu_type==SVCNAL_PREFIX || nalu_type==SVCNAL_ENHANCEMENT)
	{
		uint8_t nalu_byte1 = nalu[NALU_HEADER_LEN];
		uint8_t nalu_byte2 = nalu[NALU_HEADER_LEN+1];
		uint8_t nalu_byte3 = nalu[NALU_HEADER_LEN+2];
		uint8_t svc_flag = (nalu_byte1 & 0x80)>>7;
		currDid = (nalu_byte2 & 0x70)>>4; // *xxx ****
		currQid = (nalu_byte2 & 0x0F);     //**** xxxx
		currTid = (nalu_byte3 & 0xE0)>>5; // xxx* ****
	}


	fragment.SvcLayerInfo[layer_index] = currDid<<2 + currTid;

	switch(nalu_type)
	{
	case  SVCNAL_SPS:
	case  SVCNAL_SUBSET_SPS:
		layInfo += 0;
		break;
	case  SVCNAL_PPS:
		layInfo += (pps_id << 16);
		break;
	case SVCNAL_PREFIX:
		layInfo += (currDid<<16) + currTid;
		prefix_nalu = true;
		break;
	case  SVCNAL_ENHANCEMENT:
		layInfo += (currDid<<16) + currTid;
		break;
	default:
		break;
	}
}
}  // namespace webrtc
