#ifndef _SER_PHONECALL_H
#define _SER_PHONECALL_H

#include "serprivate.h"
#include <time.h>

//sean ice
#include "ice.h"

//#if !defined(NO_VOIP_FUNCTION)
//class VoeObserver;
//class RecordVoip;
//
//#ifdef VIDEO_ENABLED
//class VideoFrameDeliver;
//class VieObserver;
////#include "receive_statistics_proxy.h"
//#endif
//
//#endif

class ServiceCore;
struct _SerPhoneCall;
typedef struct _SerPhoneCall  SerPhoneCall;

static const int serphone_call_magic=0x3343;

typedef enum _SerphoneCallState{
	LinphoneCallIdle,					/**<Initial call state */
	LinphoneCallIncomingReceived, /**<This is a new incoming call */
	LinphoneCallOutgoingInit, /**<An outgoing call is started */
	LinphoneCallOutgoingProgress, /**<An outgoing call is in progress */
	LinphoneCallOutgoingProceeding, /**<An outgoing call is in proceeding */
	LinphoneCallOutgoingRinging, /**<An outgoing call is ringing at remote end */
	LinphoneCallOutgoingEarlyMedia, /**<An outgoing call is proposed early media */
	LinphoneCallConnected, /**<Connected, the call is answered */
	LinphoneCallStreamsRunning, /**<The media streams are established and running*/
	LinphoneCallPausing, /**<The call is pausing at the initiative of local end */
	LinphoneCallPaused, /**< The call is paused, remote end has accepted the pause */
	LinphoneCallResuming, /**<The call is being resumed by local end*/
    LinphoneCallResumed, /**<The call is has been resumed by local end*/
	LinphoneCallRefered, /**<The call is being transfered to another party, resulting in a new outgoing call to follow immediately*/
	LinphoneCallError, /**<The call encountered an error*/
	LinphoneCallEnd, /**<The call ended normally*/
	LinphoneCallPausedByRemote, /**<The call is paused by remote end*/
	LinphoneCallUpdatedByRemote, /**<The call's parameters change is requested by remote end, used for example when video is added by remote */
	LinphoneCallIncomingEarlyMedia, /**<We are proposing early media to an incoming call */
    LinphoneCallUpdating, /**<A call update has been initiated by us */
	LinphoneCallUpdated, /**<The remote accepted the call update initiated by us */
    LinphoneCallUpdatedRemoteVideoratio,
    LinphoneCallUpdatedRemoteVideoratioVideoConference,
    LinphoneCallUpdatedAudioDestinationChanged,/**Change audio destination*/
    LinphoneCallUpdatedVideoDestinationChanged,/**Change video destination*/
	LinphoneCallReleased /**< The call object is no more retained by the core */
    
} SerphoneCallState;

/**
 * Enum representing the direction of a call.
 * @ingroup call_logs
**/
enum _SerphoneCallDir {
	LinphoneCallOutgoing, /**< outgoing calls*/
	LinphoneCallIncoming  /**< incoming calls*/
};

/**
 * Typedef for enum
 * @ingroup call_logs
**/
typedef enum _SerphoneCallDir SerphoneCallDir;


struct _SerphoneCallParams{
	SerPhoneCall *referer; /*in case this call creation is consecutive to an incoming transfer, this points to the original call */
	int audio_bw; /* bandwidth limit for audio stream */
	SerphoneMediaEncryption media_encryption;
	PayloadType *audio_codec;
	PayloadType *video_codec;
	bool_t has_video;
	bool_t real_early_media; /*send real media even during early media (for outgoing calls)*/
	bool_t in_conference; /*in conference mode */
	bool_t pad;
	char *invite_userdata;
    char *group_id;
//haiyuntong
#ifdef HAIYUNTONG
    char *akey;
    char *bkey;
    char *confKey;
#endif
};

typedef struct _SerphoneCallParams SerphoneCallParams;
/**
 * Enum representing the status of a call
 * @ingroup call_logs
**/
typedef enum _SerphoneCallStatus {
	LinphoneCallSuccess, /**< The call was sucessful*/
	LinphoneCallAborted, /**< The call was aborted */
	LinphoneCallMissed, /**< The call was missed (unanswered)*/
	LinphoneCallDeclined /**< The call was declined, either locally or by remote end*/
} SerphoneCallStatus;

/**
 * Structure representing a call log.
 *
 * @ingroup call_logs
 *
**/
typedef struct _SerphoneCallLog{
	SerphoneCallDir dir; /**< The direction of the call*/
	SerphoneCallStatus status; /**< The status of the call*/
	SerphoneAddress *from; /**<Originator of the call as a LinphoneAddress object*/
	SerphoneAddress *to; /**<Destination of the call as a LinphoneAddress object*/
	char start_date[128]; /**<Human readable string containg the start date*/
	int duration; /**<Duration of the call in seconds*/
	char *refkey;
	void *user_pointer;
	rtp_stats_t local_stats;
	rtp_stats_t remote_stats;
	float quality;
    int video_enabled;
	class SerphoneCore *lc;
} SerphoneCallLog;

typedef struct _SerPhoneCall
{
	int magic; /*used to distinguish from proxy config*/
	class ServiceCore * core;
	SalMediaDescription *localdesc;
	SalMediaDescription *resultdesc;
	SerphoneCallDir dir;
	struct _SerPhoneCall *referer; /*when this call is the result of a transfer, referer is set to the original call that caused the transfer*/
	struct _RtpProfile *audio_profile;
	struct _RtpProfile *video_profile;
	struct _SerphoneCallLog *log;
	SalOp *op;
	SalOp *ping_op;
	char localip[SERPHONE_IPADDR_SIZE]; /* our best guess for local ipaddress for this call */
	time_t start_time; /*time at which the call was initiated*/
	time_t media_start_time; /*time at which it was accepted, media streams established*/
	SerphoneCallState	state;
	SerphoneCallState   transfer_state; /*idle if no transfer*/
	SerphoneReason reason;
	int refcnt;
	void * user_pointer;
	int audio_port;
	int video_port;
	char *refer_to;
	SerphoneCallParams params;
	SerphoneCallParams current_params;
	SerphoneCallParams remote_params;
	int up_bw; /*upload bandwidth setting at the time the call is started. Used to detect if it changes during a call */
	int audio_bw;	/*upload bandwidth used by audio */
	bool_t refer_pending;
	bool_t media_pending;
	bool_t audio_muted;
	bool_t camera_active;
	bool_t all_muted; /*this flag is set during early medias*/
	bool_t playing_ringbacktone;
	bool_t owns_call_log;
	bool_t ringing_beep; /* whether this call is ringing through an already existent current call*/

	char *auth_token;
	bool_t videostream_encrypted;
	bool_t audiostream_encrypted;
	bool_t auth_token_verified;
	bool_t defer_update;
	bool_t was_automatically_paused;
////media attribute
	bool_t m_audiostream_flag;
	bool_t callConnected;

/////////////////////////
	int   m_AudioChannelID;
	int   m_VideoChannelID;
//    int   m_VideoChannelID1;
//    int   m_VideoChannelID2;
	int   m_CaptureDeviceId;
	int   m_desktopShareDeviceId;
    
	int  m_selfSSRC;
	int  m_partnerSSRC;

	//bool_t audio_nack_enabled;
	//bool_t video_nack_enabled;

    //sean ice
    int ping_time;
    SerPhoneCallStats stats[2];
    IceSession *ice_session;
    StunCandidate ac,vc; /*audio video ip/port discovered by STUN*/
	struct _AudioStream *audiostream;  /**/
	struct _VideoStream *videostream;
    OrtpEvQueue *videostream_app_evq;
    OrtpEvQueue *audiostream_app_evq;
    unsigned int remote_session_id;
	unsigned int remote_session_ver;
    bool_t ping_replied;
    
    char _user_call_id[9];
    
//#if !defined(NO_VOIP_FUNCTION)
//	VoeObserver *voe_observer;
//    RecordVoip *record_voip;
//    
//#ifdef VIDEO_ENABLED
//	VideoFrameDeliver *deliver_frame;
//	VieObserver *vie_observer;
//#endif
//#endif
}SerPhoneCall;

SerphoneCallLog * serphone_call_log_new(SerPhoneCall *call,SerphoneAddress *local, SerphoneAddress * remote);
void serphone_call_init_common(SerPhoneCall *call,SerphoneAddress *from, SerphoneAddress *to);
//////////////////////log/////////////////////////////
SerphoneCallLog *serphone_call_get_call_log(const SerPhoneCall *call);
void serphone_call_log_completed(SerPhoneCall *call);
void serphone_call_log_destroy(SerphoneCallLog **cl);
//////////////////////////////transfer ////////////////
const char *serphone_call_get_refer_to(const SerPhoneCall *call);
void serphone_call_set_transfer_state(SerPhoneCall* call,SerphoneCallState state);
SerphoneCallState serphone_call_get_transfer_state(SerPhoneCall *call);
bool_t serphone_call_has_transfer_pending(const SerPhoneCall *call);
//////////////////////////////state/////////////
void serphone_call_set_state(SerPhoneCall *call,SerphoneCallState cstate, const char *message);

////////////////////call address
const SerphoneAddress * serphone_call_get_remote_address(const SerPhoneCall *call);
char *serphone_call_get_remote_address_as_string(const SerPhoneCall *call);

////////////////////call attribute
ServiceCore *serphone_call_get_core(const SerPhoneCall *call);
SerphoneCallDir serphone_call_get_dir(const SerPhoneCall *call);
SerPhoneCall * serphone_call_ref(SerPhoneCall *obj);
void serphone_call_unref(SerPhoneCall *obj);
void serphone_call_destroy(SerPhoneCall *obj);
bool_t serphone_call_asked_to_autoanswer(SerPhoneCall *call);
int serphone_call_get_duration(const SerPhoneCall *call);
SerPhoneCall *serphone_call_get_replaced_call(SerPhoneCall *call);
void *serphone_call_get_user_pointer(SerPhoneCall *call);
void serphone_call_set_user_pointer(SerPhoneCall *call, void *user_pointer);
SerphoneCallState serphone_call_get_state(const SerPhoneCall *call);
SerPhoneCall * is_a_linphone_call(void *user_pointer);
void serphone_call_fix_call_parameters(SerPhoneCall *call);
RtpProfile *make_profile(SerPhoneCall *call, const SalMediaDescription *md, const SalStreamDescription *desc, int *used_pt);
void serphone_core_update_allocated_audio_bandwidth_in_call(SerPhoneCall *call, const PayloadType *pt);

/////////////////////string
const char *serphone_call_state_to_string(SerphoneCallState cs);
void call_logs_write_to_config_file(ServiceCore *lc);


#endif
