//
//  H264_dec.h
//  video_coding
//
//  Created by Lee Sean on 13-1-30.
//
//
    
#ifndef __video_coding__H264_dec__
#define __video_coding__H264_dec__

#include "video_codec_interface.h"
#include "typedefs.h"

extern "C"
{
#if defined(_WIN32)
    #include "libavcodec/avcodec.h"
#elif MAC_IPHONE
    #include "libavcodec_ios/avcodec.h"
#else
    #include "libavcodec/avcodec.h"
#endif
}

namespace cloopenwebrtc
{
    class H264Decoder : public VideoDecoder
    {
    public:
        static H264Decoder* Create();
        
        virtual ~H264Decoder();
        
        // Initialize the decoder.
        // The user must notify the codec of width and height values.
        //
        // Return value         :  WEBRTC_VIDEO_CODEC_OK.
        //                        <0 - Errors
        virtual WebRtc_Word32 InitDecode(const VideoCodec* codecSettings, WebRtc_Word32 /*numberOfCores*/);
        
        virtual WebRtc_Word32 SetCodecConfigParameters(const WebRtc_UWord8* /*buffer*/, WebRtc_Word32 /*size*/){return WEBRTC_VIDEO_CODEC_OK;};
        
        // Decode encoded image (as a part of a video stream). The decoded image
        // will be returned to the user through the decode complete callback.
        //
        // Input:
        //          - inputImage        : Encoded image to be decoded
        //          - missingFrames     : True if one or more frames have been lost
        //                                since the previous decode call.
        //          - codecSpecificInfo : pointer to specific codec data
        //          - renderTimeMs      : Render time in Ms
        //
        // Return value                 : WEBRTC_VIDEO_CODEC_OK if OK
        //                                 <0 - Error
        virtual WebRtc_Word32 Decode(
                                     const EncodedImage& inputImage,
                                     bool missingFrames,
                                     const RTPFragmentationHeader* fragmentation,
                                     const CodecSpecificInfo* /*codecSpecificInfo*/,
                                     WebRtc_Word64 /*renderTimeMs*/);
        
        // Register a decode complete callback object.
        //
        // Input:
        //          - callback         : Callback object which handles decoded images.
        //
        // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
        virtual WebRtc_Word32 RegisterDecodeCompleteCallback(DecodedImageCallback* callback);
        
        // Free decoder memory.
        //
        // Return value                : WEBRTC_VIDEO_CODEC_OK if OK
        //                                  <0 - Error
        virtual WebRtc_Word32 Release();
        
        // Reset decoder state and prepare for a new call.
        //
        // Return value         :  WEBRTC_VIDEO_CODEC_OK.
        //                          <0 - Error
        virtual WebRtc_Word32 Reset();
        void pgm_save(FILE *f,unsigned char *buf,int wrap, int xsize,int ysize);
	private:
		int getVopType( const void *p, int len );
		int get_nal_type( void *p, int len );
		int ReturnFrame(const AVFrame* img,
			uint32_t timeStamp,
			int64_t ntp_time_ms);
    private:
        H264Decoder();
        void reInitDec();
        void printTime();

		int PrepareRawImage(AVFrame *pframe);
        
        I420VideoFrame              _decodedImage;
		VideoCodec					_decoderSetting;
        bool                        _inited;
        DecodedImageCallback*       _decodeCompleteCallback;
       
        //For H264
        AVCodecContext *_codecContext;
        AVFrame *pFrame_;

		int							_numberOfCores;
#ifdef _WIN32
		FILE		*_fragFIle;
#endif
#if 0
        FILE *fout;
        FILE *fout2;
#endif
        
    };
    
} // namespace cloopenwebrtc
    
#endif /* defined(__video_coding__H264_dec__) */

