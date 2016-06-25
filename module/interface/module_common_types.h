/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULE_COMMON_TYPES_H
#define MODULE_COMMON_TYPES_H

#include <cstring> // memcpy
#include <assert.h>

#include "constructormagic.h"
#include "typedefs.h"
#include "common_types.h"

#include "critical_section_wrapper.h"

#ifdef _WIN32
    #pragma warning(disable:4351)       // remove warning "new behavior: elements of array
                                        // 'array' will be default initialized"
#endif

namespace cloopenwebrtc
{
//struct RTPHeader
//{
//    bool           markerBit;
//    WebRtc_UWord8  payloadType;
//    WebRtc_UWord16 sequenceNumber;
//    WebRtc_UWord32 timestamp;
//    WebRtc_UWord32 ssrc;
//    WebRtc_UWord8  numCSRCs;
//    WebRtc_UWord32 arrOfCSRCs[kRtpCsrcSize];
//    WebRtc_UWord8  paddingLength;
//    WebRtc_UWord16 headerLength;
//};
//
//struct RTPHeaderExtension
//{
//    WebRtc_Word32  transmissionTimeOffset;
//};

struct RTPAudioHeader
{
    WebRtc_UWord8  numEnergy;                         // number of valid entries in arrOfEnergy
    WebRtc_UWord8  arrOfEnergy[kRtpCsrcSize];   // one energy byte (0-9) per channel
    bool           isCNG;                             // is this CNG
    WebRtc_UWord8  channel;                           // number of channels 2 = stereo
};

enum {kNoPictureId = -1};
enum {kNoTl0PicIdx = -1};
enum {kNoTemporalIdx = -1};
enum {kNoKeyIdx = -1};
enum {kNoSimulcastIdx = 0};

struct RTPVideoHeaderVP8
{
    void InitRTPVideoHeaderVP8()
    {
        nonReference = false;
        pictureId = kNoPictureId;
        tl0PicIdx = kNoTl0PicIdx;
        temporalIdx = kNoTemporalIdx;
        layerSync = false;
        keyIdx = kNoKeyIdx;
        partitionId = 0;
        beginningOfPartition = false;
        frameWidth = 0;
        frameHeight = 0;
    }

    bool           nonReference;   // Frame is discardable.
    WebRtc_Word16  pictureId;      // Picture ID index, 15 bits;
                                   // kNoPictureId if PictureID does not exist.
    WebRtc_Word16  tl0PicIdx;      // TL0PIC_IDX, 8 bits;
                                   // kNoTl0PicIdx means no value provided.
    WebRtc_Word8   temporalIdx;    // Temporal layer index, or kNoTemporalIdx.
    bool           layerSync;      // This frame is a layer sync frame.
                                   // Disabled if temporalIdx == kNoTemporalIdx.
    int            keyIdx;         // 5 bits; kNoKeyIdx means not used.
    int            partitionId;    // VP8 partition ID
    bool           beginningOfPartition;  // True if this packet is the first
                                          // in a VP8 partition. Otherwise false
    int            frameWidth;     // Exists for key frames.
    int            frameHeight;    // Exists for key frames.
};
struct RTPVideoHeaderH264
{
    void InitRTPVideoHeaderH264()
    {
        nonReference = false;
        pictureId = kNoPictureId;
    }
    
    bool           last;                //the last packet in frame
    bool           nonReference;   // Frame is discardable.
    WebRtc_Word16  pictureId;      // Picture ID index, 15 bits;
                                                 // kNoPictureId if PictureID does not exist.
	bool stap_a;
	bool single_nalu;
	bool fu_a;
};

struct RTPVideoHeaderH264SVC
{
	void InitRTPVideoHeaderH264SVC()
	{
		nonReference = false;
		pictureId = kNoPictureId;
	}

	bool           last;                //the last packet in frame
	bool           nonReference;   // Frame is discardable.
	WebRtc_Word16  pictureId;      // Picture ID index, 15 bits;
	// kNoPictureId if PictureID does not exist.
	bool			stap_a;
	bool			single_nalu;
	bool			fu_a;

	bool			relayE;
};


union RTPVideoTypeHeader
{
    RTPVideoHeaderVP8       VP8;
    RTPVideoHeaderH264      H264;
	RTPVideoHeaderH264SVC   H264SVC;
};

//enum RtpVideoCodecTypes
//{
//    kRTPVideoGeneric	= 0,
//    kRTPVideoVP8		= 8,
//    kRTPVideoNoVideo	= 10,
//    kRTPVideoFEC		= 11,
//    kRTPVideoI420		= 12,
//    kRTPVideoH264		= 13,
//	kRTPVideoH264SVC	= 14
//};

enum RtpVideoCodecTypes {
	kRtpVideoNone,
	kRtpVideoGeneric,
	kRtpVideoVp8,
	kRtpVideoH264
};

struct RTPVideoHeader
{
    WebRtc_UWord16          width;                  // size
    WebRtc_UWord16          height;

    bool                    isFirstPacket;   // first packet in frame
    WebRtc_UWord8           simulcastIdx;    // Index if the simulcast encoder creating
                                             // this frame, 0 if not using simulcast.
    RtpVideoCodecTypes      codec;
    RTPVideoTypeHeader      codecHeader;
};
union RTPTypeHeader
{
    RTPAudioHeader  Audio;
    RTPVideoHeader  Video;
};

struct WebRtcRTPHeader
{
    RTPHeader       header;
    FrameType       frameType;
    RTPTypeHeader   type;
    RTPHeaderExtension extension;
//    sean add begin 0922 FU_A
    bool isFua;
//    sean add end 0922 FU_A

	bool isStap_a;
	bool isSingle_nalu;

	// NTP time of the capture time in local timebase in milliseconds.
	int64_t ntp_time_ms;
};
//
//struct WebRtcRTPHeader {
//	RTPHeader header;
//	FrameType frameType;
//	RTPTypeHeader type;
//	// NTP time of the capture time in local timebase in milliseconds.
//	int64_t ntp_time_ms;
//};


class RTPFragmentationHeader
{
public:
    RTPFragmentationHeader() :
        fragmentationVectorSize(0),
        fragmentationOffset(NULL),
        fragmentationLength(NULL),
        fragmentationTimeDiff(NULL),
        fragmentationPlType(NULL),
		SvcFrameInfo(NULL),
		SvcLayerInfo(NULL),
		SvcFrameNum(NULL)
    {};

    ~RTPFragmentationHeader()
    {
        delete [] fragmentationOffset;
        delete [] fragmentationLength;
        delete [] fragmentationTimeDiff;
        delete [] fragmentationPlType;
		delete [] SvcFrameInfo;
		delete [] SvcLayerInfo;
		delete [] SvcFrameNum;
    }
	void CopyFrom(const RTPFragmentationHeader& src) {
		if (this == &src) {
			return;
		}

		if (src.fragmentationVectorSize != fragmentationVectorSize) {
			// new size of vectors

			// delete old
			delete[] fragmentationOffset;
			fragmentationOffset = NULL;
			delete[] fragmentationLength;
			fragmentationLength = NULL;
			delete[] fragmentationTimeDiff;
			fragmentationTimeDiff = NULL;
			delete[] fragmentationPlType;
			fragmentationPlType = NULL;
			delete [] SvcFrameInfo;
			SvcFrameInfo = NULL;
			delete [] SvcLayerInfo;
			SvcLayerInfo = NULL;
			delete [] SvcFrameNum;
			SvcFrameNum = NULL;

			if (src.fragmentationVectorSize > 0) {
				// allocate new
				if (src.fragmentationOffset) {
					fragmentationOffset = new size_t[src.fragmentationVectorSize];
				}
				if (src.fragmentationLength) {
					fragmentationLength = new size_t[src.fragmentationVectorSize];
				}
				if (src.fragmentationTimeDiff) {
					fragmentationTimeDiff = new uint16_t[src.fragmentationVectorSize];
				}
				if (src.fragmentationPlType) {
					fragmentationPlType = new uint8_t[src.fragmentationVectorSize];
				}
				if (src.SvcFrameInfo)
				{
					SvcFrameInfo = new uint8_t[src.fragmentationVectorSize];
				}
				if (src.SvcLayerInfo)
				{
					SvcLayerInfo = new uint8_t[src.fragmentationVectorSize];
				}
				if (src.SvcFrameNum)
				{
					SvcFrameNum = new uint16_t[src.fragmentationVectorSize];
				}
			}
			// set new size
			fragmentationVectorSize = src.fragmentationVectorSize;
		}

		if (src.fragmentationVectorSize > 0) {
			// copy values
			if (src.fragmentationOffset) {
				memcpy(fragmentationOffset, src.fragmentationOffset,
					src.fragmentationVectorSize * sizeof(size_t));
			}
			if (src.fragmentationLength) {
				memcpy(fragmentationLength, src.fragmentationLength,
					src.fragmentationVectorSize * sizeof(size_t));
			}
			if (src.fragmentationTimeDiff) {
				memcpy(fragmentationTimeDiff, src.fragmentationTimeDiff,
					src.fragmentationVectorSize * sizeof(uint16_t));
			}
			if (src.fragmentationPlType) {
				memcpy(fragmentationPlType, src.fragmentationPlType,
					src.fragmentationVectorSize * sizeof(uint8_t));
			}
			if (src.fragmentationPlType) {
				fragmentationPlType = new uint8_t[src.fragmentationVectorSize];
			}
			if (src.SvcFrameInfo)
			{
				memcpy(SvcFrameInfo, src.SvcFrameInfo,
					src.fragmentationVectorSize * sizeof(uint8_t));
			}
			if (src.SvcLayerInfo)
			{
				memcpy(SvcLayerInfo, src.SvcLayerInfo,
					src.fragmentationVectorSize * sizeof(uint8_t));
			}
			if (src.SvcFrameNum)
			{
				memcpy(SvcFrameNum, src.SvcFrameNum,
					src.fragmentationVectorSize * sizeof(uint16_t));
			}
		}
	}
    RTPFragmentationHeader& operator=(const RTPFragmentationHeader& header)
    {
        if(this == &header)
        {
            return *this;
        }

        if(header.fragmentationVectorSize != fragmentationVectorSize)
        {
            // new size of vectors

            // delete old
            delete [] fragmentationOffset;
            fragmentationOffset = NULL;
            delete [] fragmentationLength;
            fragmentationLength = NULL;
            delete [] fragmentationTimeDiff;
            fragmentationTimeDiff = NULL;
            delete [] fragmentationPlType;
            fragmentationPlType = NULL;
			delete [] SvcFrameInfo;
			SvcFrameInfo = NULL;
			delete [] SvcLayerInfo;
			SvcLayerInfo = NULL;
			delete [] SvcFrameNum;
			SvcFrameNum = NULL;

            if(header.fragmentationVectorSize > 0)
            {
                // allocate new
                if(header.fragmentationOffset)
                {
                    fragmentationOffset = new size_t[header.fragmentationVectorSize];
                }
                if(header.fragmentationLength)
                {
                    fragmentationLength = new size_t[header.fragmentationVectorSize];
                }
                if(header.fragmentationTimeDiff)
                {
                    fragmentationTimeDiff = new WebRtc_UWord16[header.fragmentationVectorSize];
                }
                if(header.fragmentationPlType)
                {
                    fragmentationPlType = new WebRtc_UWord8[header.fragmentationVectorSize];
                }
				if (header.SvcFrameInfo)
				{
					SvcFrameInfo = new WebRtc_UWord8[header.fragmentationVectorSize];
				}
				if (header.SvcLayerInfo)
				{
					SvcLayerInfo = new WebRtc_UWord8[header.fragmentationVectorSize];
				}
				if (header.SvcFrameNum)
				{
					SvcFrameNum = new WebRtc_UWord16[header.fragmentationVectorSize];
				}				
            }
            // set new size
            fragmentationVectorSize =   header.fragmentationVectorSize;
        }

        if(header.fragmentationVectorSize > 0)
        {
            // copy values
            if(header.fragmentationOffset)
            {
                memcpy(fragmentationOffset, header.fragmentationOffset,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord32));
            }
            if(header.fragmentationLength)
            {
                memcpy(fragmentationLength, header.fragmentationLength,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord32));
            }
            if(header.fragmentationTimeDiff)
            {
                memcpy(fragmentationTimeDiff, header.fragmentationTimeDiff,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord16));
            }
            if(header.fragmentationPlType)
            {
                memcpy(fragmentationPlType, header.fragmentationPlType,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord8));
            }
			if (header.SvcFrameInfo)
			{
				memcpy(SvcFrameInfo, header.SvcFrameInfo,
						header.fragmentationVectorSize * sizeof(WebRtc_UWord8));
			}
			if (header.SvcLayerInfo)
			{
				memcpy(SvcLayerInfo, header.SvcLayerInfo,
					header.fragmentationVectorSize * sizeof(WebRtc_UWord8));
			}
			if (header.SvcFrameNum)
			{
				memcpy(SvcFrameNum, header.SvcFrameNum,
					header.fragmentationVectorSize * sizeof(WebRtc_UWord16));
			}		
        }
        return *this;
    }
    void VerifyAndAllocateFragmentationHeader( const WebRtc_UWord16 size)
    {
        if( fragmentationVectorSize < size)
        {
            WebRtc_UWord16 oldVectorSize = fragmentationVectorSize;
            {
                // offset
                size_t* oldOffsets = fragmentationOffset;
                fragmentationOffset = new size_t[size];
                memset(fragmentationOffset+oldVectorSize, 0,
                       sizeof(WebRtc_UWord32)*(size-oldVectorSize));
                // copy old values
                memcpy(fragmentationOffset,oldOffsets, sizeof(WebRtc_UWord32) * oldVectorSize);
                delete[] oldOffsets;
            }
            // length
            {
                size_t* oldLengths = fragmentationLength;
                fragmentationLength = new size_t[size];
                memset(fragmentationLength+oldVectorSize, 0,
                       sizeof(WebRtc_UWord32) * (size- oldVectorSize));
                memcpy(fragmentationLength, oldLengths,
                       sizeof(WebRtc_UWord32) * oldVectorSize);
                delete[] oldLengths;
            }
            // time diff
            {
                WebRtc_UWord16* oldTimeDiffs = fragmentationTimeDiff;
                fragmentationTimeDiff = new WebRtc_UWord16[size];
                memset(fragmentationTimeDiff+oldVectorSize, 0,
                       sizeof(WebRtc_UWord16) * (size- oldVectorSize));
                memcpy(fragmentationTimeDiff, oldTimeDiffs,
                       sizeof(WebRtc_UWord16) * oldVectorSize);
                delete[] oldTimeDiffs;
            }
            // payload type
            {
                WebRtc_UWord8* oldTimePlTypes = fragmentationPlType;
                fragmentationPlType = new WebRtc_UWord8[size];
                memset(fragmentationPlType+oldVectorSize, 0,
                       sizeof(WebRtc_UWord8) * (size- oldVectorSize));
                memcpy(fragmentationPlType, oldTimePlTypes,
                       sizeof(WebRtc_UWord8) * oldVectorSize);
                delete[] oldTimePlTypes;
            }
			//svc layer info
			{
				WebRtc_UWord8* oldSvcFrameInfo = SvcFrameInfo;
				SvcFrameInfo = new WebRtc_UWord8[size];
				memset(SvcFrameInfo+oldVectorSize, 0,
						 sizeof(WebRtc_UWord8) * (size- oldVectorSize));
				memcpy(SvcFrameInfo, oldSvcFrameInfo,
					sizeof(WebRtc_UWord8) * oldVectorSize);
				delete[] oldSvcFrameInfo;
			}
			{
				WebRtc_UWord8* oldSvcLayerInfo = SvcLayerInfo;
				SvcLayerInfo = new WebRtc_UWord8[size];
				memset(SvcLayerInfo+oldVectorSize, 0,
					sizeof(WebRtc_UWord8) * (size- oldVectorSize));
				memcpy(SvcLayerInfo, oldSvcLayerInfo,
					sizeof(WebRtc_UWord8) * oldVectorSize);
				delete[] oldSvcLayerInfo;
			}
			{
				WebRtc_UWord16* oldSvcFrameNum= SvcFrameNum;
				SvcFrameNum = new WebRtc_UWord16[size];
				memset(SvcFrameNum+oldVectorSize, 0,
					sizeof(WebRtc_UWord16) * (size- oldVectorSize));
				memcpy(SvcFrameNum, oldSvcFrameNum,
					sizeof(WebRtc_UWord16) * oldVectorSize);
				delete[] oldSvcFrameNum;
			}

            fragmentationVectorSize = size;
        }
    }

    WebRtc_UWord16    fragmentationVectorSize;    // Number of fragmentations
    size_t*   fragmentationOffset;        // Offset of pointer to data for each fragm.
    size_t*   fragmentationLength;        // Data size for each fragmentation
    WebRtc_UWord16*   fragmentationTimeDiff;      // Timestamp difference relative "now" for
                                                  // each fragmentation
    WebRtc_UWord8*    fragmentationPlType;        // Payload type of each fragmentation
	WebRtc_UWord8*		SvcFrameInfo;
	WebRtc_UWord8*		SvcLayerInfo;		  // add by ylr
	WebRtc_UWord16*		SvcFrameNum;
};

struct RTCPVoIPMetric
{
    // RFC 3611 4.7
    WebRtc_UWord8     lossRate;
    WebRtc_UWord8     discardRate;
    WebRtc_UWord8     burstDensity;
    WebRtc_UWord8     gapDensity;
    WebRtc_UWord16    burstDuration;
    WebRtc_UWord16    gapDuration;
    WebRtc_UWord16    roundTripDelay;
    WebRtc_UWord16    endSystemDelay;
    WebRtc_UWord8     signalLevel;
    WebRtc_UWord8     noiseLevel;
    WebRtc_UWord8     RERL;
    WebRtc_UWord8     Gmin;
    WebRtc_UWord8     Rfactor;
    WebRtc_UWord8     extRfactor;
    WebRtc_UWord8     MOSLQ;
    WebRtc_UWord8     MOSCQ;
    WebRtc_UWord8     RXconfig;
    WebRtc_UWord16    JBnominal;
    WebRtc_UWord16    JBmax;
    WebRtc_UWord16    JBabsMax;
};

// Types for the FEC packet masks. The type |kFecMaskRandom| is based on a
// random loss model. The type |kFecMaskBursty| is based on a bursty/consecutive
// loss model. The packet masks are defined in
// modules/rtp_rtcp/fec_private_tables_random(bursty).h
enum FecMaskType {
	kFecMaskRandom,
	kFecMaskBursty,
};

// Struct containing forward error correction settings.
struct FecProtectionParams {
	int fec_rate;
	bool use_uep_protection;
	int max_fec_frames;
	FecMaskType fec_mask_type;
};

// Interface used by the CallStats class to distribute call statistics.
// Callbacks will be triggered as soon as the class has been registered to a
// CallStats object using RegisterStatsObserver.
class CallStatsObserver {
public:
	virtual void OnRttUpdate(int64_t rtt_ms) = 0;

	virtual ~CallStatsObserver() {}
};


// class describing a complete, or parts of an encoded frame.
class EncodedVideoData
{
public:
    EncodedVideoData() :
        payloadType(0),
        timeStamp(0),
        renderTimeMs(0),
        encodedWidth(0),
        encodedHeight(0),
        completeFrame(false),
        missingFrame(false),
        payloadData(NULL),
        payloadSize(0),
        bufferSize(0),
        fragmentationHeader(),
        frameType(kVideoFrameDelta),
        codec(kVideoCodecUnknown)
    {};

    EncodedVideoData(const EncodedVideoData& data)
    {
        payloadType         = data.payloadType;
        timeStamp           = data.timeStamp;
        renderTimeMs        = data.renderTimeMs;
        encodedWidth        = data.encodedWidth;
        encodedHeight       = data.encodedHeight;
        completeFrame       = data.completeFrame;
        missingFrame        = data.missingFrame;
        payloadSize         = data.payloadSize;
        fragmentationHeader = data.fragmentationHeader;
        frameType           = data.frameType;
        codec               = data.codec;
        if (data.payloadSize > 0)
        {
            payloadData = new WebRtc_UWord8[data.payloadSize];
            memcpy(payloadData, data.payloadData, data.payloadSize);
        }
        else
        {
            payloadData = NULL;
        }
    }


    ~EncodedVideoData()
    {
        delete [] payloadData;
    };

    EncodedVideoData& operator=(const EncodedVideoData& data)
    {
        if (this == &data)
        {
            return *this;
        }
        payloadType         = data.payloadType;
        timeStamp           = data.timeStamp;
        renderTimeMs        = data.renderTimeMs;
        encodedWidth        = data.encodedWidth;
        encodedHeight       = data.encodedHeight;
        completeFrame       = data.completeFrame;
        missingFrame        = data.missingFrame;
        payloadSize         = data.payloadSize;
        fragmentationHeader = data.fragmentationHeader;
        frameType           = data.frameType;
        codec               = data.codec;
        if (data.payloadSize > 0)
        {
            delete [] payloadData;
            payloadData = new WebRtc_UWord8[data.payloadSize];
            memcpy(payloadData, data.payloadData, data.payloadSize);
            bufferSize = data.payloadSize;
        }
        return *this;
    };
    void VerifyAndAllocate( const WebRtc_UWord32 size)
    {
        if (bufferSize < size)
        {
            WebRtc_UWord8* oldPayload = payloadData;
            payloadData = new WebRtc_UWord8[size];
            memcpy(payloadData, oldPayload, sizeof(WebRtc_UWord8) * payloadSize);

            bufferSize = size;
            delete[] oldPayload;
        }
    }

    WebRtc_UWord8               payloadType;
    WebRtc_UWord32              timeStamp;
    WebRtc_Word64               renderTimeMs;
    WebRtc_UWord32              encodedWidth;
    WebRtc_UWord32              encodedHeight;
    bool                        completeFrame;
    bool                        missingFrame;
    WebRtc_UWord8*              payloadData;
    WebRtc_UWord32              payloadSize;
    WebRtc_UWord32              bufferSize;
    RTPFragmentationHeader      fragmentationHeader;
    FrameType                   frameType;
    VideoCodecType              codec;
};

struct VideoContentMetrics {
  VideoContentMetrics()
      : motion_magnitude(0.0f),
        spatial_pred_err(0.0f),
        spatial_pred_err_h(0.0f),
        spatial_pred_err_v(0.0f) {
  }

  void Reset() {
    motion_magnitude = 0.0f;
    spatial_pred_err = 0.0f;
    spatial_pred_err_h = 0.0f;
    spatial_pred_err_v = 0.0f;
  }
  float motion_magnitude;
  float spatial_pred_err;
  float spatial_pred_err_h;
  float spatial_pred_err_v;
};

/*************************************************
 *
 * VideoFrame class
 *
 * The VideoFrame class allows storing and
 * handling of video frames.
 *
 *
 *************************************************/
class VideoFrame
{
public:
    VideoFrame();
    ~VideoFrame();
    /**
    * Verifies that current allocated buffer size is larger than or equal to the input size.
    * If the current buffer size is smaller, a new allocation is made and the old buffer data
    * is copied to the new buffer.
    * Buffer size is updated to minimumSize.
    */
    WebRtc_Word32 VerifyAndAllocate(const WebRtc_UWord32 minimumSize);
    /**
    *    Update length of data buffer in frame. Function verifies that new length is less or
    *    equal to allocated size.
    */
    WebRtc_Word32 SetLength(const WebRtc_UWord32 newLength);
    /*
    *    Swap buffer and size data
    */
    WebRtc_Word32 Swap(WebRtc_UWord8*& newMemory,
                       WebRtc_UWord32& newLength,
                       WebRtc_UWord32& newSize);
    /*
    *    Swap buffer and size data
    */
    WebRtc_Word32 SwapFrame(VideoFrame& videoFrame);
    /**
    *    Copy buffer: If newLength is bigger than allocated size, a new buffer of size length
    *    is allocated.
    */
    WebRtc_Word32 CopyFrame(const VideoFrame& videoFrame);
    /**
    *    Copy buffer: If newLength is bigger than allocated size, a new buffer of size length
    *    is allocated.
    */
    WebRtc_Word32 CopyFrame(WebRtc_UWord32 length, const WebRtc_UWord8* sourceBuffer);
    /**
    *    Delete VideoFrame and resets members to zero
    */
    void Free();
    /**
    *   Set frame timestamp (90kHz)
    */
    void SetTimeStamp(const WebRtc_UWord32 timeStamp) {_timeStamp = timeStamp;}
    /**
    *   Get pointer to frame buffer
    */
    WebRtc_UWord8*    Buffer() const {return _buffer;}

    WebRtc_UWord8*&   Buffer() {return _buffer;}

    /**
    *   Get allocated buffer size
    */
    WebRtc_UWord32    Size() const {return _bufferSize;}
    /**
    *   Get frame length
    */
    WebRtc_UWord32    Length() const {return _bufferLength;}
    /**
    *   Get frame timestamp (90kHz)
    */
    WebRtc_UWord32    TimeStamp() const {return _timeStamp;}
    /**
    *   Get frame width
    */
    WebRtc_UWord32    Width() const {return _width;}
    /**
    *   Get frame height
    */
    WebRtc_UWord32    Height() const {return _height;}
    /**
    *   Set frame width
    */
    void   SetWidth(const WebRtc_UWord32 width)  {_width = width;}
    /**
    *   Set frame height
    */
    void  SetHeight(const WebRtc_UWord32 height) {_height = height;}
    /**
    *   Set render time in miliseconds
    */
    void SetRenderTime(const WebRtc_Word64 renderTimeMs) {_renderTimeMs = renderTimeMs;}
    /**
    *  Get render time in miliseconds
    */
    WebRtc_Word64    RenderTimeMs() const {return _renderTimeMs;}

private:
    void Set(WebRtc_UWord8* buffer,
             WebRtc_UWord32 size,
             WebRtc_UWord32 length,
             WebRtc_UWord32 timeStamp);

    WebRtc_UWord8*          _buffer;          // Pointer to frame buffer
    WebRtc_UWord32          _bufferSize;      // Allocated buffer size
    WebRtc_UWord32          _bufferLength;    // Length (in bytes) of buffer
    WebRtc_UWord32          _timeStamp;       // Timestamp of frame (90kHz)
    WebRtc_UWord32          _width;
    WebRtc_UWord32          _height;
    WebRtc_Word64           _renderTimeMs;
    
    //Sean add begin 20140227 windows8 crash
    cloopenwebrtc::CriticalSectionWrapper*    _criticalSectionVideoFrame;
    //Sean add end 20140227 windows8 crash
}; // end of VideoFrame class declaration

// inline implementation of VideoFrame class:
inline
VideoFrame::VideoFrame():
    _buffer(0),
    _bufferSize(0),
    _bufferLength(0),
    _timeStamp(0),
    _width(0),
    _height(0),
    _renderTimeMs(0),
    //Sean add begin 20140227 windows8 crash
    _criticalSectionVideoFrame(
                               CriticalSectionWrapper::CreateCriticalSection())
    //Sean add end 20140227 windows8 crash
{
    //
}
inline
VideoFrame::~VideoFrame()
{
	{
		CriticalSectionScoped lock(_criticalSectionVideoFrame);
		if(_buffer)
		{
			delete [] _buffer;
			_buffer = NULL;
		}
	}
	delete _criticalSectionVideoFrame;
	_criticalSectionVideoFrame = NULL;
}


inline
WebRtc_Word32
VideoFrame::VerifyAndAllocate(const WebRtc_UWord32 minimumSize)
{
    CriticalSectionScoped lock(_criticalSectionVideoFrame);
    if (minimumSize < 1)
    {
        return -1;
    }
    if(minimumSize > _bufferSize)
    {
        // create buffer of sufficient size
        WebRtc_UWord8* newBufferBuffer = new WebRtc_UWord8[minimumSize];
        if(_buffer)
        {
            // copy old data
            memcpy(newBufferBuffer, _buffer, _bufferSize);
            delete [] _buffer;
        }
        else
        {
            memset(newBufferBuffer, 0, minimumSize * sizeof(WebRtc_UWord8));
        }
        _buffer = newBufferBuffer;
        _bufferSize = minimumSize;
    }
    return 0;
}

inline
WebRtc_Word32
VideoFrame::SetLength(const WebRtc_UWord32 newLength)
{
    if (newLength >_bufferSize )
    { // can't accomodate new value
        return -1;
    }
     _bufferLength = newLength;
     return 0;
}

inline
WebRtc_Word32
VideoFrame::SwapFrame(VideoFrame& videoFrame)
{
    WebRtc_UWord32 tmpTimeStamp  = _timeStamp;
    WebRtc_UWord32 tmpWidth      = _width;
    WebRtc_UWord32 tmpHeight     = _height;
    WebRtc_Word64  tmpRenderTime = _renderTimeMs;

    _timeStamp = videoFrame._timeStamp;
    _width = videoFrame._width;
    _height = videoFrame._height;
    _renderTimeMs = videoFrame._renderTimeMs;

    videoFrame._timeStamp = tmpTimeStamp;
    videoFrame._width = tmpWidth;
    videoFrame._height = tmpHeight;
    videoFrame._renderTimeMs = tmpRenderTime;

    return Swap(videoFrame._buffer, videoFrame._bufferLength, videoFrame._bufferSize);
}

inline
WebRtc_Word32
VideoFrame::Swap(WebRtc_UWord8*& newMemory, WebRtc_UWord32& newLength, WebRtc_UWord32& newSize)
{
    WebRtc_UWord8* tmpBuffer = _buffer;
    WebRtc_UWord32 tmpLength = _bufferLength;
    WebRtc_UWord32 tmpSize = _bufferSize;
    _buffer = newMemory;
    _bufferLength = newLength;
    _bufferSize = newSize;
    newMemory = tmpBuffer;
    newLength = tmpLength;
    newSize = tmpSize;
    return 0;
}

inline
WebRtc_Word32
VideoFrame::CopyFrame(WebRtc_UWord32 length, const WebRtc_UWord8* sourceBuffer)
{
    if (length > _bufferSize)
    {
        WebRtc_Word32 ret = VerifyAndAllocate(length);
        if (ret < 0)
        {
            return ret;
        }
    }
     memcpy(_buffer, sourceBuffer, length);
    _bufferLength = length;
    return 0;
}

inline
WebRtc_Word32
VideoFrame::CopyFrame(const VideoFrame& videoFrame)
{
    if(CopyFrame(videoFrame.Length(), videoFrame.Buffer()) != 0)
    {
        return -1;
    }
    _timeStamp = videoFrame._timeStamp;
    _width = videoFrame._width;
    _height = videoFrame._height;
    _renderTimeMs = videoFrame._renderTimeMs;
    return 0;
}

inline
void
VideoFrame::Free()
{
    CriticalSectionScoped lock(_criticalSectionVideoFrame);
    _timeStamp = 0;
    _bufferLength = 0;
    _bufferSize = 0;
    _height = 0;
    _width = 0;
    _renderTimeMs = 0;

    if(_buffer)
    {
        delete [] _buffer;
        _buffer = NULL;
    }
}


/* This class holds up to 60 ms of super-wideband (32 kHz) stereo audio. It
 * allows for adding and subtracting frames while keeping track of the resulting
 * states.
 *
 * Notes
 * - The total number of samples in |data_| is
 *   samples_per_channel_ * num_channels_
 *
 * - Stereo data is interleaved starting with the left channel.
 *
 * - The +operator assume that you would never add exactly opposite frames when
 *   deciding the resulting state. To do this use the -operator.
 */
class AudioFrame {
 public:
  // Stereo, 32 kHz, 60 ms (2 * 32 * 60)
  static const int kMaxDataSizeSamples = 3840;

  enum VADActivity {
    kVadActive = 0,
    kVadPassive = 1,
    kVadUnknown = 2
  };
  enum SpeechType {
    kNormalSpeech = 0,
    kPLC = 1,
    kCNG = 2,
    kPLCCNG = 3,
    kUndefined = 4
  };

  AudioFrame();
  virtual ~AudioFrame() {}

  // Resets all members to their default state (except does not modify the
  // contents of |data_|).
  void Reset();

  // |interleaved_| is not changed by this method.
  void UpdateFrame(int id, uint32_t timestamp, const int16_t* data,
                   int samples_per_channel, int sample_rate_hz,
                   SpeechType speech_type, VADActivity vad_activity,
                   int num_channels = 1, uint32_t energy = -1);

  AudioFrame& Append(const AudioFrame& rhs);

  void CopyFrom(const AudioFrame& src);

  void Mute();

  AudioFrame& operator>>=(const int rhs);
  AudioFrame& operator+=(const AudioFrame& rhs);
  AudioFrame& operator-=(const AudioFrame& rhs);

  int id_;
  // RTP timestamp of the first sample in the AudioFrame.
  uint32_t timestamp_;
  // Time since the first frame in milliseconds.
  // -1 represents an uninitialized value.
  int64_t elapsed_time_ms_;
  // NTP time of the estimated capture time in local timebase in milliseconds.
  // -1 represents an uninitialized value.
  int64_t ntp_time_ms_;
  int16_t data_[kMaxDataSizeSamples];
  int samples_per_channel_;
  int sample_rate_hz_;
  int num_channels_;
  SpeechType speech_type_;
  VADActivity vad_activity_;
  // Note that there is no guarantee that |energy_| is correct. Any user of this
  // member must verify that the value is correct.
  // TODO(henrike) Remove |energy_|.
  // See https://code.google.com/p/webrtc/issues/detail?id=3315.
  uint32_t energy_;
  bool interleaved_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioFrame);
};

inline AudioFrame::AudioFrame()
    : data_() {
  Reset();
}

inline void AudioFrame::Reset() {
  id_ = -1;
  // TODO(wu): Zero is a valid value for |timestamp_|. We should initialize
  // to an invalid value, or add a new member to indicate invalidity.
  timestamp_ = 0;
  elapsed_time_ms_ = -1;
  ntp_time_ms_ = -1;
  samples_per_channel_ = 0;
  sample_rate_hz_ = 0;
  num_channels_ = 0;
  speech_type_ = kUndefined;
  vad_activity_ = kVadUnknown;
  energy_ = 0xffffffff;
  interleaved_ = true;
}

inline void AudioFrame::UpdateFrame(int id, uint32_t timestamp,
                                    const int16_t* data,
                                    int samples_per_channel, int sample_rate_hz,
                                    SpeechType speech_type,
                                    VADActivity vad_activity, int num_channels,
                                    uint32_t energy) {
  id_ = id;
  timestamp_ = timestamp;
  samples_per_channel_ = samples_per_channel;
  sample_rate_hz_ = sample_rate_hz;
  speech_type_ = speech_type;
  vad_activity_ = vad_activity;
  num_channels_ = num_channels;
  energy_ = energy;

  const int length = samples_per_channel * num_channels;
  assert(length <= kMaxDataSizeSamples && length >= 0);
  if (data != NULL) {
    memcpy(data_, data, sizeof(int16_t) * length);
  } else {
    memset(data_, 0, sizeof(int16_t) * length);
  }
}

inline void AudioFrame::CopyFrom(const AudioFrame& src) {
  if (this == &src) return;

  id_ = src.id_;
  timestamp_ = src.timestamp_;
  elapsed_time_ms_ = src.elapsed_time_ms_;
  ntp_time_ms_ = src.ntp_time_ms_;
  samples_per_channel_ = src.samples_per_channel_;
  sample_rate_hz_ = src.sample_rate_hz_;
  speech_type_ = src.speech_type_;
  vad_activity_ = src.vad_activity_;
  num_channels_ = src.num_channels_;
  energy_ = src.energy_;
  interleaved_ = src.interleaved_;

  const int length = samples_per_channel_ * num_channels_;
  assert(length <= kMaxDataSizeSamples && length >= 0);
  memcpy(data_, src.data_, sizeof(int16_t) * length);
}

inline void AudioFrame::Mute() {
  memset(data_, 0, samples_per_channel_ * num_channels_ * sizeof(int16_t));
}

inline AudioFrame& AudioFrame::operator>>=(const int rhs) {
  assert((num_channels_ > 0) && (num_channels_ < 3));
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;

  for (int i = 0; i < samples_per_channel_ * num_channels_; i++) {
    data_[i] = static_cast<int16_t>(data_[i] >> rhs);
  }
  return *this;
}

inline AudioFrame& AudioFrame::Append(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  assert(interleaved_ == rhs.interleaved_);
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;
  if (num_channels_ != rhs.num_channels_) return *this;

  if ((vad_activity_ == kVadActive) || rhs.vad_activity_ == kVadActive) {
    vad_activity_ = kVadActive;
  } else if (vad_activity_ == kVadUnknown || rhs.vad_activity_ == kVadUnknown) {
    vad_activity_ = kVadUnknown;
  }
  if (speech_type_ != rhs.speech_type_) {
    speech_type_ = kUndefined;
  }

  int offset = samples_per_channel_ * num_channels_;
  for (int i = 0; i < rhs.samples_per_channel_ * rhs.num_channels_; i++) {
    data_[offset + i] = rhs.data_[i];
  }
  samples_per_channel_ += rhs.samples_per_channel_;
  return *this;
}

inline AudioFrame& AudioFrame::operator+=(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  assert(interleaved_ == rhs.interleaved_);
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;
  if (num_channels_ != rhs.num_channels_) return *this;

  bool noPrevData = false;
  if (samples_per_channel_ != rhs.samples_per_channel_) {
    if (samples_per_channel_ == 0) {
      // special case we have no data to start with
      samples_per_channel_ = rhs.samples_per_channel_;
      noPrevData = true;
    } else {
      return *this;
    }
  }

  if ((vad_activity_ == kVadActive) || rhs.vad_activity_ == kVadActive) {
    vad_activity_ = kVadActive;
  } else if (vad_activity_ == kVadUnknown || rhs.vad_activity_ == kVadUnknown) {
    vad_activity_ = kVadUnknown;
  }

  if (speech_type_ != rhs.speech_type_) speech_type_ = kUndefined;

  if (noPrevData) {
    memcpy(data_, rhs.data_,
           sizeof(int16_t) * rhs.samples_per_channel_ * num_channels_);
  } else {
    // IMPROVEMENT this can be done very fast in assembly
    for (int i = 0; i < samples_per_channel_ * num_channels_; i++) {
      int32_t wrapGuard =
          static_cast<int32_t>(data_[i]) + static_cast<int32_t>(rhs.data_[i]);
      if (wrapGuard < -32768) {
        data_[i] = -32768;
      } else if (wrapGuard > 32767) {
        data_[i] = 32767;
      } else {
        data_[i] = (int16_t)wrapGuard;
      }
    }
  }
  energy_ = 0xffffffff;
  return *this;
}

inline AudioFrame& AudioFrame::operator-=(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  assert(interleaved_ == rhs.interleaved_);
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;

  if ((samples_per_channel_ != rhs.samples_per_channel_) ||
      (num_channels_ != rhs.num_channels_)) {
    return *this;
  }
  if ((vad_activity_ != kVadPassive) || rhs.vad_activity_ != kVadPassive) {
    vad_activity_ = kVadUnknown;
  }
  speech_type_ = kUndefined;

  for (int i = 0; i < samples_per_channel_ * num_channels_; i++) {
    int32_t wrapGuard =
        static_cast<int32_t>(data_[i]) - static_cast<int32_t>(rhs.data_[i]);
    if (wrapGuard < -32768) {
      data_[i] = -32768;
    } else if (wrapGuard > 32767) {
      data_[i] = 32767;
    } else {
      data_[i] = (int16_t)wrapGuard;
    }
  }
  energy_ = 0xffffffff;
  return *this;
}

inline bool IsNewerSequenceNumber(uint16_t sequence_number,
	uint16_t prev_sequence_number) {
		return sequence_number != prev_sequence_number &&
			static_cast<uint16_t>(sequence_number - prev_sequence_number) < 0x8000;
}

inline bool IsNewerTimestamp(uint32_t timestamp, uint32_t prev_timestamp) {
	return timestamp != prev_timestamp &&
		static_cast<uint32_t>(timestamp - prev_timestamp) < 0x80000000;
}

inline uint16_t LatestSequenceNumber(uint16_t sequence_number1,
	uint16_t sequence_number2) {
		return IsNewerSequenceNumber(sequence_number1, sequence_number2)
			? sequence_number1
			: sequence_number2;
}

inline uint32_t LatestTimestamp(uint32_t timestamp1, uint32_t timestamp2) {
	return IsNewerTimestamp(timestamp1, timestamp2) ? timestamp1 : timestamp2;
}
} // namespace cloopenwebrtc

#endif // MODULE_COMMON_TYPES_H
