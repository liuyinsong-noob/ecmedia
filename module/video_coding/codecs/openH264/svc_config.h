#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_SVC_CONFIG_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_SVC_CONFIG_H_

#include "codec_api.h"
#include "codec_def.h" 

#define EPSN (0.000001f) // (1e-6) // desired float precision

inline int FillSpecificParameters_Ratio_4to3 (SEncParamExt& sParam) {
		int iIndexLayer = 0;
		sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_BASELINE;
		sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 160;
		sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 120;
		sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 7.5f;
		sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 64000;
		sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
		sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;

		++ iIndexLayer;
		sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_SCALABLE_BASELINE;
		sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 320;
		sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 240;
		sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 15.0f;
		sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 160000;
		sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
		sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;

		++ iIndexLayer;
		sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_SCALABLE_BASELINE;
		sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 640;
		sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 480;
		sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 30.0f;
		sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 512000;
		sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
		sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
		//sParam.sSpatialLayers[iIndexLayer].sSliceCfg.sSliceArgument.uiSliceNum = 1;

		++ iIndexLayer;
		sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_SCALABLE_BASELINE;
		sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 1280;
		sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 960;
		sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 30.0f;
		sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 1500000;
		sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
		sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
		//sParam.sSpatialLayers[iIndexLayer].sSliceCfg.sSliceArgument.uiSliceNum = 1;

		float fMaxFr = sParam.sSpatialLayers[sParam.iSpatialLayerNum - 1].fFrameRate;
		for (int32_t i = sParam.iSpatialLayerNum - 2; i >= 0; -- i) {
			if (sParam.sSpatialLayers[i].fFrameRate > fMaxFr + EPSN)
				fMaxFr = sParam.sSpatialLayers[i].fFrameRate;
		}
		sParam.fMaxFrameRate = fMaxFr;

		return 0;
}

inline int FillSpecificParameters_Ratio_16to9 (SEncParamExt& sParam) {
	int iIndexLayer = 0;
	sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_BASELINE;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 160;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 90;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 7.5f;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 64000;
	sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;


	++ iIndexLayer;
	sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_SCALABLE_BASELINE;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 320;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 180;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 15.0f;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 160000;
	sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;

	++ iIndexLayer;
	sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_SCALABLE_BASELINE;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 640;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 360;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 30.0f;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 512000;
	sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
	//sParam.sSpatialLayers[iIndexLayer].sSliceCfg.sSliceArgument.uiSliceNum = 1;

	++ iIndexLayer;
	sParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_SCALABLE_BASELINE;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth        = 1280;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight       = 720;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate         = 30.0f;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = 1500000;
	sParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate    = UNSPECIFIED_BIT_RATE;
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
	//sParam.sSpatialLayers[iIndexLayer].sSliceCfg.sSliceArgument.uiSliceNum = 1;

	float fMaxFr = sParam.sSpatialLayers[sParam.iSpatialLayerNum - 1].fFrameRate;
	for (int32_t i = sParam.iSpatialLayerNum - 2; i >= 0; -- i) {
		if (sParam.sSpatialLayers[i].fFrameRate > fMaxFr + EPSN)
			fMaxFr = sParam.sSpatialLayers[i].fFrameRate;
	}
	sParam.fMaxFrameRate = fMaxFr;
	return 0;
}

#endif