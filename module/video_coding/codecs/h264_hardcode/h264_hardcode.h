//
//  h264_hardcode.hpp
//  video_coding
//
//  Created by SeanLee on 16/6/23.
//
//

#ifndef h264_hardcode_hpp
#define h264_hardcode_hpp


#include <MacTypes.h>
#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CoreMedia.h>
#include "video_encoder.h"


namespace cloopenwebrtc {
        
        // VideoToolbox implementation of the media::cast::VideoEncoder interface.
        // VideoToolbox makes no guarantees that it is thread safe, so this object is
        // pinned to the thread on which it is constructed. Supports changing frame
        // sizes directly. Implements the base::PowerObserver interface to reset the
        // compression session when the host process is suspended.
        class H264VideoToolboxEncoder : public VideoEncoder {
        public:
            static H264VideoToolboxEncoder* Create();
            
            virtual WebRtc_Word32 InitEncode(const VideoCodec* codecSetting, int32_t numberOfCores, size_t maxPayloadSize);
            // Returns true if the current platform and system configuration supports
            // using H264VideoToolboxEncoder with the given |video_config|.
//            static bool IsSupported(const VideoSenderConfig& video_config);
            
            
            
            
            ~H264VideoToolboxEncoder() final;
            
            // media::cast::VideoEncoder implementation
            virtual WebRtc_Word32 Encode(const I420VideoFrame& inputImage,
                                  const CodecSpecificInfo* codecSpecificInfo,
                                  const std::vector<VideoFrameType>* frame_types) final;
            
            virtual WebRtc_Word32 RegisterEncodeCompleteCallback(EncodedImageCallback* callback);
            
            // Free encoder memory.
            //
            // Return value                : WEBRTC_VIDEO_CODEC_OK if OK, < 0 otherwise.
            virtual WebRtc_Word32 Release();
            
            virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 packetLoss,
                                                       int64_t rtt);
            
            virtual WebRtc_Word32 SetRates(WebRtc_UWord32 newBitRate, WebRtc_UWord32 frameRate);
            
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
            H264VideoToolboxEncoder();
            // VideoFrameFactory tied to the VideoToolbox encoder.
//            class VideoFrameFactoryImpl;
            
            // Reset the encoder's compression session by destroying the existing one
            // using DestroyCompressionSession() and creating a new one. The new session
            // is configured using ConfigureCompressionSession().
            void ResetCompressionSession();
            
            // Configure the current compression session using current encoder settings.
            void ConfigureCompressionSession(int width, int height);
            
            // Destroy the current compression session if any. Blocks until all pending
            // frames have been flushed out (similar to EmitFrames without doing any
            // encoding work).
            void DestroyCompressionSession();
            
            static bool CopyEncodedImage(RTPFragmentationHeader &fragment, int num_nals, int64_t ts, bool keyFrame, char *buffer, size_t length, bool startcode);
            // Compression session callback function to handle compressed frames.
            static void CompressionCallback(void* encoder_opaque,
                                            void* request_opaque,
                                            OSStatus status,
                                            VTEncodeInfoFlags info,
                                            CMSampleBufferRef sbuf);
            

            // The compression session.
            VTCompressionSessionRef compression_session_;
            
            // Video frame factory tied to the encoder.
//            scoped_refptr<VideoFrameFactoryImpl> video_frame_factory_;
            
            // Force next frame to be a keyframe.
            bool encode_next_frame_as_keyframe_;
            
            
            static EncodedImage encoded_image_;
            static EncodedImageCallback* encoded_complete_callback_;
            bool inited_;
            uint16_t picture_id_;
            bool periodicKeyFrames_;
            uint32_t framenum_;
            int32_t bitrate;
            int32_t fps;
            int mode;
            bool generate_keyframe;
            int num_of_cores_;
            static VideoCodec codec_;
            static FILE *debug_file_;
            static int64_t size_;
            dispatch_queue_t aQueue;
            int frame_count_;
            int bitrate_;
            NSLock *lock;
            
            CVPixelBufferPoolRef pixelBufferPool_;
            CFDictionaryRef _bufferPoolAuxAttributes;
            DISALLOW_COPY_AND_ASSIGN(H264VideoToolboxEncoder);
        };
    
}  // namespace cloopenwebrtc

#endif /* h264_hardcode_hpp */
