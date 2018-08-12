#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_DEC_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_OPENH264_DEC_IMPL_H_

#ifdef WIN32
#include <stdio.h>
#endif

#include "codec_api.h"
#include "codec_def.h" 
#include "openH264.h"
#include "common_types.h"


namespace yuntongxunwebrtc{

class OpenH264DecoderImpl : public OpenH264Decoder {
public:
	enum {
		MAX_ENCODED_IMAGE_SIZE = 102400
	};

	OpenH264DecoderImpl();

	virtual ~OpenH264DecoderImpl();

	// Initialize the decoder.
	//
	// Return value         :  WEBRTC_VIDEO_CODEC_OK.
	//                        <0 - Errors:
	//                                  WEBRTC_VIDEO_CODEC_ERROR
	virtual int InitDecode(const VideoCodec* inst, int number_of_cores);

	// Decode encoded image (as a part of a video stream). The decoded image
	// will be returned to the user through the decode complete callback.
	//
	// Input:
	//          - input_image         : Encoded image to be decoded
	//          - missing_frames      : True if one or more frames have been lost
	//                                  since the previous decode call.
	//          - fragmentation       : Specifies the start and length of each H264
	//                                  partition.
	//          - codec_specific_info : pointer to specific codec data
	//          - render_time_ms      : Render time in Ms
	//
	// Return value                 : WEBRTC_VIDEO_CODEC_OK if OK
	//                                <0 - Errors:
	//                                      WEBRTC_VIDEO_CODEC_ERROR
	//                                      WEBRTC_VIDEO_CODEC_ERR_PARAMETER
	virtual int Decode(const EncodedImage& input_image,
		bool missing_frames,
		const RTPFragmentationHeader* fragmentation,
		const CodecSpecificInfo* codec_specific_info,
		int64_t /*render_time_ms*/);

	// Register a decode complete callback object.
	//
	// Input:
	//          - callback         : Callback object which handles decoded images.
	//
	// Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
	virtual int RegisterDecodeCompleteCallback(DecodedImageCallback* callback);

	// Free decoder memory.
	//
	// Return value                : WEBRTC_VIDEO_CODEC_OK if OK
	//                               <0 - Errors:
	//                                      WEBRTC_VIDEO_CODEC_ERROR
	virtual int Release();

	// Reset decoder state and prepare for a new call.
	//
	// Return value         : WEBRTC_VIDEO_CODEC_OK.
	//                        <0 - Errors:
	//                                  WEBRTC_VIDEO_CODEC_UNINITIALIZED
	//                                  WEBRTC_VIDEO_CODEC_ERROR
	virtual int Reset();

	// Create a copy of the codec and its internal state.
	//
	// Return value                : A copy of the instance if OK, NULL otherwise.
	virtual VideoDecoder* Copy();

	bool PartitionNeedDecode(uint8_t* nalu_with_start_code, int length, int Tid, int Did, bool &prefix_nalu);
	int PrepareRawImage(uint8_t** data, SBufferInfo *info);

private:
	RawImage				decoded_image_;
	ISVCDecoder				*decoder_;
	DecodedImageCallback	*decode_complete_callback_;
	VideoCodec				codec_;
	bool					inited_;
	uint8_t					number_of_cores_;
#ifdef WIN32
	FILE *_decodedYUV;
	FILE *_fragFIle;
	FILE *_distractorFile;
#endif
	unsigned long long uiTimeStamp;
};  // end of H264Decoder classclass H264DecoderImpl : public H264Decoder {
 
}//namespace yuntongxunwebrtc

#endif