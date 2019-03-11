#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H_

#include "video_codec_interface.h"
#include "module_common_types.h"
#include <stdio.h>

extern "C"
{
#if MAC_IPHONE
#include "x264_ios.h"
#else
#include "x264.h"
#endif
}

//#define HAVE_H264_BITSTREAM

namespace yuntongxunwebrtc
{
    
//#define MAX_RTP_PKT_LENGTH      1400
//enum NALU_TYPE{
//    SINGLE,
//    STAP_A,
//    FU_A
//};
//    
//typedef struct {    
//    //byte 0 
//    unsigned char TYPE:5;    
//    unsigned char NRI:2;    
//    unsigned char F:1;   
//} NALU_HEADER; /**//* 1 BYTES */
//    
//typedef struct {
//    //byte 0    
//    unsigned char TYPE:5;    
//    unsigned char NRI:2;
//    unsigned char F:1;
//} FU_INDICATOR; /**//* 1 BYTES */
//    
//typedef struct {
//    //byte 0
//    unsigned char TYPE:5;    
//    unsigned char R:1;    
//    unsigned char E:1;    
//    unsigned char S:1;    
//} FU_HEADER; /**//* 1 BYTES */
//
//typedef struct
//{     
//    int nalu_start_index;
//    int nalus_num;
//    int nalus_len;
//    int nalu_type;
//    bool last;
//} NALU_t;
//    
//typedef void (*H264EncodeCallBack)(const EncodedVideoData& frameToStore, void *userdata);
//
///* the goal of this small object is to tell when to send I frames at startup:
// at 2 and 4 seconds*/
//typedef struct VideoStarter{
//    uint64_t next_time;
//    int i_frame_count;
//}VideoStarter;

class H264Encoder : public VideoEncoder {
public:
    static H264Encoder* Create();
        
    virtual ~H264Encoder();

    // Initialize the encoder with the information from the VideoCodec.
    //
    // Input:
    //          - codecSettings     : Codec settings
    //          - numberOfCores     : Number of cores available for the encoder
    //          - maxPayloadSize    : The maximum size each payload is allowed
    //                                to have. Usually MTU - overhead.
    //
    // Return value                 : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 InitEncode(const VideoCodec* codecSettings, int32_t numberOfCores, size_t maxPayloadSize);

    // Encode an I420 image (as a part of a video stream). The encoded image
    // will be returned to the user through the encode complete callback.
    //
    // Input:
    //          - inputImage        : Image to be encoded
    //          - codecSpecificInfo : Pointer to codec specific data
    //          - frameType         : The frame type to encode
    //
    // Return value                 : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 Encode(const I420VideoFrame& inputImage,
                                 const CodecSpecificInfo* codecSpecificInfo,
                                 const std::vector<VideoFrameType>* frame_types);

    // Register an encode complete callback object.
    //
    // Input:
    //          - callback         : Callback object which handles encoded images.
    //
    // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 RegisterEncodeCompleteCallback(EncodedImageCallback* callback);

    // Free encoder memory.
    //
    // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 Release();

    // Inform the encoder about the packet loss and round trip time on the
    // network used to decide the best pattern and signaling.
    //
    //          - packetLoss       : Fraction lost (loss rate in percent =
    //                               100 * packetLoss / 255)
    //          - rtt              : Round-trip time in milliseconds
    //
    // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 packetLoss,
                                               int64_t rtt);

    // Inform the encoder about the new target bit rate.
    //
    //          - newBitRate       : New target bit rate
    //          - frameRate        : The target frame rate
    //
    // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 SetRates(WebRtc_UWord32 newBitRate, WebRtc_UWord32 frameRate,
                                   uint32_t minBitrate_kbit = 0, uint32_t maxBitrate_kbit = 0);

    // Use this function to enable or disable periodic key frames. Can be useful for codecs
    // which have other ways of stopping error propagation.
    //
    //          - enable           : Enable or disable periodic key frames
    //
    // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 SetPeriodicKeyFrames(bool enable);

    // Codec configuration data to send out-of-band, i.e. in SIP call setup
    //
    //          - buffer           : Buffer pointer to where the configuration data
    //                               should be stored
    //          - size             : The size of the buffer in bytes
    //
    // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
    virtual WebRtc_Word32 CodecConfigParameters(WebRtc_UWord8* /*buffer*/, WebRtc_Word32 /*size*/);
    
private:
    H264Encoder();
	void SetX264EncodeParameters(x264_param_t &params,VideoCodecMode mode, VideoCodecType type);
	void InitializeX264Pic(const I420VideoFrame& input_image, x264_picture_t &xpic, x264_picture_t &oxpic, VideoFrameType frame_type);
	bool CopyEncodedImage(RTPFragmentationHeader &fragment, void *xnals, int num_nals, void *opic, const I420VideoFrame &input_image, VideoCodecMode mode);
    EncodedImage encoded_image_;
    EncodedImageCallback* encoded_complete_callback_;
    VideoCodec codec_;  
	int num_of_cores_;
    bool inited_;
    uint16_t picture_id_;
    bool periodicKeyFrames_;
 
    int bitrate;
	float fps;
    int mode;
    uint32_t framenum_;
    bool generate_keyframe;    
    x264_t * encoder_;   
    bool stap_a_allowed;
    int count;
#ifdef HAVE_H264_BITSTREAM
	FILE *_bitStreamBeforeSend;
#endif

};

}


#endif  //WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H_
