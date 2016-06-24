#ifndef _SDK_COMMON_H_
#define _SDK_COMMON_H_

typedef struct {
	int width;
	int height;
	int maxfps;
} CameraCapability;

typedef struct{
	int index;
	char name[256];
	int capabilityCount;
	CameraCapability *capability;
    char id[256];
}  CameraInfo ;

typedef struct{
	int index;
	char name[128];  //utf8
    char guid[128];	  //utf8
}  SpeakerInfo ;

typedef struct{
	int index;
	char name[128];   //utf8
	char guid[128];    //utf8
}  MicroPhoneInfo ;

enum {
	ROTATE_AUTO,
	ROTATE_0,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270
};
	
typedef struct 
{
    unsigned short fractionLost;
    unsigned int cumulativeLost;
    unsigned int extendedMax;
    unsigned int jitterSamples;
    int rttMs;
    unsigned int bytesSent;
    unsigned int packetsSent;
    unsigned int bytesReceived;
    unsigned int packetsReceived;
} CallStatisticsInfo;

enum
{
    AUDIO_AGC,
    AUDIO_EC,
    AUDIO_NS
};

enum NsMode    // type of Noise Suppression
{
    kNsUnchanged = 0,   // previously set mode
    kNsDefault,         // platform default
    kNsConference,      // conferencing default
    kNsLowSuppression,  // lowest suppression
    kNsModerateSuppression,
    kNsHighSuppression,
    kNsVeryHighSuppression,     // highest suppression
};

enum AgcMode                  // type of Automatic Gain Control
{
    kAgcUnchanged = 0,        // previously set mode
    kAgcDefault,              // platform default
    // adaptive mode for use when analog volume control exists (e.g. for
    // PC softphone)
    kAgcAdaptiveAnalog,
    // scaling takes place in the digital domain (e.g. for conference servers
    // and embedded devices)
    kAgcAdaptiveDigital,
    // can be used on embedded devices where the capture signal level
    // is predictable
    kAgcFixedDigital
};

// EC modes
enum EcMode                  // type of Echo Control
{
    kEcUnchanged = 0,          // previously set mode
    kEcDefault,                // platform default
    kEcConference,             // conferencing default (aggressive AEC)
    kEcAec,                    // Acoustic Echo Cancellation
    kEcAecm,                   // AEC mobile
};

//enum  {
//    codec_iLBC,
//    codec_G729,
//    codec_PCMU,
//    codec_PCMA,
//    codec_VP8,
//    codec_H264,
//    codec_SILK8K,
//    codec_SILK12K,
//    codec_SILK16K
//};

enum  {
    codec_PCMU = 0,
    codec_G729,
    codec_OPUS48,
    codec_OPUS16,
    codec_OPUS8,
    codec_VP8,
    codec_H264,
};

typedef enum _CCPClientFirewallPolicy {
    SerphonePolicyNoFirewall = 0,
    SerphonePolicyUseIce
    
} CCPClientFirewallPolicy;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
#endif