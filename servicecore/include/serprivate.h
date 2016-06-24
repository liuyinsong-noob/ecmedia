#ifndef SER_PRIVATE_H
#define SER_PRIVATE_H

#include "sometools.h"
#include "salpr.h"
#include "lpconfig.h"

#include "ice.h"

#define SERPHONE_IPADDR_SIZE 64
#define SERPHONE_HOSTNAME_SIZE 128

/* Serphone's version number */
#define SERPHONE_VERSION "1.0.0"

#ifndef NB_MAX_CALLS
#define NB_MAX_CALLS	(10)
#endif

#define AUDIO_RTP_PACKET_TIMEOUT 20  //seconds

typedef struct SalAddress SerphoneAddress;
class ServiceCore;
class SerphoneFriend;
struct _SerphoneChatRoom;
typedef struct _SerphoneChatRoom SerphoneChatRoom;
struct _SerphoneAuthInfo;
typedef struct _SerphoneAuthInfo SerphoneAuthInfo;
/**
 * Represents a buddy, all presence actions like subscription and status change notification are performed on this object
 */

/**
 * Enum describing failure reasons.
 * @ingroup initializing
**/
#ifndef OLDERRORCODE
enum _SerphoneReason{
    SerphoneReasonUserDefinedError = 1000, //For special purpose, for XINWEI
    SerphoneReasonNone = 175000,
    SerphoneReasonNoResponse = 175001, /**<No response received from remote*/
    SerphoneReasonBadCredentials = 175002, /**<Authentication failed due to bad or missing credentials*/
    SerphoneReasonNoVoip = 175005,
    SerphoneReasonNotFound = 175404,
    SerphoneReasonTimeout = 175408,
    SerphoneReasonCallMissed = 175409,
    SerphoneReasonBusy = 175486,
    SerphoneReasonDeclined = 175603 /**<The call has been declined*/
};
#else
enum _SerphoneReason{
    SerphoneReasonNone,
    SerphoneReasonNoResponse, /**<No response received from remote*/
    SerphoneReasonBadCredentials, /**<Authentication failed due to bad or missing credentials*/
    SerphoneReasonDeclined, /**<The call has been declined*/
    SerphoneReasonNotFound,
    SerphoneReasonCallMissed,
    SerphoneReasonBusy,
    SerphoneReasonNoVoip=10,
    SerphoneReasonUserDefinedError = 1000
};
#endif

typedef enum _SerphoneReason SerphoneReason;


enum _SerphoneThreadType {
    SerphoneAudioRtp = 1,
    SerphoneAudioRtcp,
    SerphoneVideoRtp,
    SerphoneVideoRtcp
};

typedef enum _SerphoneThreadType SerphoneThreadType;



const char *Serphone_reason_to_string(SerphoneReason err);


typedef  enum {
	/**
	 * Does not automatically accept an incoming subscription request.
	 * This policy implies that a decision has to be taken for each incoming subscription request notified by callback LinphoneCoreVTable.new_subscription_request
	 *
	 */
	LinphoneSPWait,
	/**
	 * Rejects incoming subscription request.
	 */
	LinphoneSPDeny,
	/**
	 * Automatically accepts a subscription request.
	 */
	LinphoneSPAccept
}SerphoneSubscribePolicy;

/**
 * Enum describing remote friend status
 */
typedef enum _SerphoneOnlineStatus{
	/**
	 * Offline
	 */
	LinphoneStatusOffline,
	/**
	 * Online
	 */
	LinphoneStatusOnline,
	/**
	 * Busy
	 */
	LinphoneStatusBusy,
	/**
	 * Be right back
	 */
	LinphoneStatusBeRightBack,
	/**
	 * Away
	 */
	LinphoneStatusAway,
	/**
	 * On the phone
	 */
	LinphoneStatusOnThePhone,
	/**
	 * Out to lunch
	 */
	LinphoneStatusOutToLunch,
	/**
	 * Do not disturb
	 */
	LinphoneStatusDoNotDisturb,
	/**
	 * Moved in this sate, call can be redirected if an alternate contact address has been set using function linphone_core_set_presence_info()
	 */
	LinphoneStatusMoved,
	/**
	 * Using another messaging service
	 */
	LinphoneStatusAltService,
	/**
	 * Pending
	 */
	LinphoneStatusPending,

	LinphoneStatusEnd
}SerphoneOnlineStatus;

SalPresenceStatus serphone_online_status_to_sal(SerphoneOnlineStatus os);

/**
 * SerphoneRegistrationState describes proxy registration states.
**/
typedef enum _SerphoneRegistrationState{
	LinphoneRegistrationNone, /**<Initial state for registrations */
	LinphoneRegistrationProgress, /**<Registration is in progress */
	LinphoneRegistrationOk,	/**< Registration is successful */
	LinphoneRegistrationCleared, /**< Unregistration succeeded */
	LinphoneRegistrationFailed	/**<Registration failed */
}SerphoneRegistrationState;

enum SerphoneMediaEncryption {
	LinphoneMediaEncryptionNone,
	LinphoneMediaEncryptionSRTP,
	LinphoneMediaEncryptionZRTP
};


/**
 * Human readable version of the #LinphoneRegistrationState
 * @param cs sate
 */
const char *sphone_registration_state_to_string(SerphoneRegistrationState cs);

struct _LCSipTransports{
	int udp_port;
	int tcp_port;
	int dtls_port;
	int tls_port;
};

typedef struct _LCSipTransports LCSipTransports;

typedef struct sip_config
{
	char *contact;
	char *guessed_contact;
	MSList *proxies;
	MSList *deleted_proxies;
	int inc_timeout;	/*timeout after an un-answered incoming call is rejected*/
	unsigned int keepalive_period; /* interval in ms between keep alive messages sent to the proxy server*/
	LCSipTransports transports;
	bool_t use_info;
	bool_t use_rfc2833;	/*force RFC2833 to be sent*/
	bool_t guess_hostname;
	bool_t loopback_only;
	bool_t ipv6_enabled;
	bool_t sdp_200_ack;
	bool_t register_only_when_network_is_up;
	bool_t ping_with_options;
	bool_t auto_net_state_mon;
} sip_config_t;

typedef struct rtp_config
{
	int audio_rtp_port;
	int video_rtp_port;
	int audio_jitt_comp;  /*jitter compensation*/
	int video_jitt_comp;  /*jitter compensation*/
	int nortp_timeout;
	bool_t rtp_no_xmit_on_audio_mute;
                              /* stop rtp xmit when audio muted */
}rtp_config_t;



typedef struct net_config
{
	char *nat_address; /* may be IP or host name */
	char *nat_address_ip; /* ip translated from nat_address */
	char *stun_server;
	char *relay;
	int download_bw;
	int upload_bw;
	int firewall_policy;
	int mtu;
	int down_ptime;
	bool_t nat_sdp_only;
}net_config_t;

typedef struct codecs_config
{
	MSList *audio_codecs;  /* list of audio codecs in order of preference*/
	MSList *video_codecs;	/* for later use*/
}codecs_config_t;

typedef struct video_config{
//	struct _MSWebCam *device;  //commented by zdm
	const char **cams;
//	MSVideoSize vsize;
	bool_t capture;
	bool_t show_local;
	bool_t display;
	bool_t selfview; /*during calls*/
	const char *displaytype;
}video_config_t;

typedef struct sound_config
{

//	struct _MSSndCard * ring_sndcard;	/* the playback sndcard currently used */
//	struct _MSSndCard * play_sndcard;	/* the playback sndcard currently used */
//	struct _MSSndCard * capt_sndcard; /* the capture sndcard currently used */
//	struct _MSSndCard * lsd_card; /* dummy playback card for Linphone Sound Daemon extension */
//	const char **cards;
	int latency;	/* latency in samples of the current used sound device */
	char rec_lev;
	char play_lev;
	char ring_lev;
	char soft_play_lev;
	char source;
	char *local_ring;
	char *remote_ring;
//    sean 2015
#ifdef WIN32
    char *pre_ring;
    time_t pre_ring_starttime;
    char *after_ring;
    time_t after_ring_starttime;
#endif
	char *ringback_tone;
	bool_t ec;
	bool_t ea;
	bool_t agc;
} sound_config_t;

typedef struct capability_config
{
    int localrec;  //record voice during call
    int localrecvoip;
	int hdvideo;
} capability_config_t;

typedef enum _SerphoneFirewallPolicy{
	LinphonePolicyNoFirewall,
	LinphonePolicyUseNatAddress,
	LinphonePolicyUseStun,
    //sean ice
    LinphonePolicyUseIce
    
} SerphoneFirewallPolicy;

/**
 * Structure describing policy regarding video streams establishments.
**/
struct _SerphoneVideoPolicy{
	bool_t automatically_initiate; /**<Whether video shall be automatically proposed for outgoing calls.*/ 
	bool_t automatically_accept; /**<Whether video shall be automatically accepted for incoming calls*/
	bool_t unused[2];
};

typedef struct _SerphoneVideoPolicy SerphoneVideoPolicy;


struct _SerphoneChatRoom{
	ServiceCore *lc;
	char  *peer;
	SerphoneAddress *peer_url;
	SalOp *op;
	void * user_data;
};

void serphone_chat_room_destroy1(SerphoneChatRoom **cr);
void serphone_chat_room_destroy(SerphoneChatRoom **cr);
const SerphoneAddress* serphone_chat_room_get_peer_address(SerphoneChatRoom *cr);
const char *serphone_chat_room_send_message(SerphoneChatRoom *to, const char *msg);
void serphone_chat_room_set_user_data(SerphoneChatRoom *cr, void * ud);
void * serphone_chat_room_get_user_data(SerphoneChatRoom *cr);

static const int serphone_proxy_config_magic=0x7979;

class _SerphoneProxyConfig
{
public:
	_SerphoneProxyConfig();
	~_SerphoneProxyConfig();
public:
    void serphone_proxy_config_activate_sip_setup();
	void serphone_proxy_config_register();
	char *guess_contact_for_register();
	int serphone_proxy_config_send_publish(SerphoneOnlineStatus presence_mode);
public:

    void serphone_proxy_config_init();


    int serphone_proxy_config_set_identity(const char *identity);
	void serphone_proxy_config_enable_publish(bool_t val);
    void serphone_proxy_config_set_dial_escape_plus(bool_t val);
    void serphone_proxy_config_set_dial_prefix(const char *prefix);

    SerphoneRegistrationState serhone_proxy_config_get_state();
    void serphone_proxy_config_set_state(SerphoneRegistrationState rstate, const char *message);
    bool_t serphone_proxy_config_is_registered();
    const char *serphone_proxy_config_get_domain();
	void serphone_proxy_config_apply(ServiceCore *lc);

    int serphone_proxy_config_set_route(const char *route);
	const char *serphone_proxy_config_get_route();
    const char *serphone_proxy_config_get_identity();
    bool_t serphone_proxy_config_publish_enabled();
    const char *serphone_proxy_config_get_addr();
	void serphone_proxy_config_expires(int val);
    int serphone_proxy_config_get_expires( );
    bool_t serphone_proxy_config_register_enabled();
    void serphone_proxy_config_refresh_register();
    class ServiceCore * serphone_proxy_config_get_core();
    void serphone_proxy_config_enableregister(bool_t val);
	void serphone_proxy_config_enable_register( bool_t val);
#define serphone_proxy_config_enableregister serphone_proxy_config_enable_register

    bool_t serphone_proxy_config_get_dial_escape_plus();
    const char * serphone_proxy_config_get_dial_prefix();

    SerphoneReason linphone_proxy_config_get_error();

    void serphone_proxy_config_set_sip_setup(const char *type);
/**
 * normalize a human readable phone number into a basic string. 888-444-222 becomes 888444222
 */
    int serphone_proxy_config_normalize_number(const char *username, char *result, size_t result_len);
/*
 *  attached a user data to a proxy config
 */
    void serphone_proxy_config_set_user_data(void * ud);
/*
 *  get user data to a proxy config. return null if any
 */
    void * serphone_proxy_config_get_user_data();

    int serphone_proxy_config_set_server_addr(const char *server_addr);
	SerphoneReason serphone_proxy_config_get_error();
	void serphone_proxy_config_set_error(SerphoneReason error);
	void serphone_proxy_config_edit();
	int serphone_proxy_config_done();

public:
	int magic;
	class ServiceCore *lc;
	char *reg_proxy;
	char *reg_identity;
	char *reg_route;
	char *realm;
//sean add begin 20140423 setPrivateCloud
    int port;
//sean add end 20140423 setPrivateCloud
	int expires;
	int reg_time;
	SalOp *op;
	char *type;
	struct _SipSetupContext *ssctx;
	int auth_failures;
	char *dial_prefix;
	SerphoneRegistrationState state;
	SalOp *publish_op;
	bool_t commit;
	bool_t reg_sendregister;
	bool_t publish;
	bool_t dial_escape_plus;
	void* user_data;
	time_t deletion_date;
	SerphoneReason error;
};
/**
 * The SerphoneProxyConfig object represents a proxy configuration to be used
 * by the LinphoneCore object.
 * Its fields must not be used directly in favour of the accessors methods.
 * Once created and filled properly the SerphoneProxyConfig can be given to
 * serviceCore with serphone_core_add_proxy_config().
 * This will automatically triggers the registration, if enabled.
 *
 * The proxy configuration are persistent to restarts because they are saved
 * in the configuration file. As a consequence, after serphone_core_new() there
 * might already be a list of configured proxy that can be examined with
 * serphone_core_get_proxy_config_list().
 *
 * The default proxy (see serphone_core_set_default_proxy() ) is the one of the list
 * that is used by default for calls.
**/
typedef class _SerphoneProxyConfig SerphoneProxyConfig;

SerphoneProxyConfig *serphone_proxy_config_new();
void serphone_proxy_config_destroy(SerphoneProxyConfig **obj);
SerphoneProxyConfig *serphone_proxy_config_new_from_config_file(LpConfig *config, int index);

struct _SerphoneAuthInfo
{
	char *username;
	char *realm;
	char *userid;
	char *passwd;
	char *ha1;
	int usecount;
	time_t last_use_time;
	bool_t works;
};

/**
 * SerphoneGlobalState describes the global state of the SerphoneCore object.
 * It is notified via the SerphoneCoreVTable::global_state_changed
**/
typedef enum _SerphoneGlobalState{
	LinphoneGlobalOff,
	LinphoneGlobalStartup,
	LinphoneGlobalOn,
	LinphoneGlobalShutdown
}SerphoneGlobalState;

//sean ice
typedef struct StunCandidate{
	char addr[64];
	int port;
}StunCandidate;

typedef enum StreamType {
	AudioStreamType,
	VideoStreamType
} StreamTypel;

struct _MediaStream {
	StreamTypel type;
//	MSTicker *ticker;
	RtpSession *session;
	OrtpEvQueue *evq;
//	MSFilter *rtprecv;
//	MSFilter *rtpsend;
//	MSFilter *encoder;
//	MSFilter *decoder;
//	MSFilter *voidsink;
//	MSBitrateController *rc;
	IceCheckList *ice_check_list;
//	OrtpZrtpContext *zrtp_context;
//	srtp_t srtp_session;
//	bool_t use_rc;
};

typedef struct _MediaStream MediaStream;
struct _AudioStream
{
	MediaStream ms;
//	MSFilter *soundread;
//	MSFilter *soundwrite;
//	MSFilter *dtmfgen;
//	MSFilter *dtmfgen_rtp;
//	MSFilter *plc;
//	MSFilter *ec;/*echo canceler*/
//	MSFilter *volsend,*volrecv; /*MSVolumes*/
//	MSFilter *read_resampler;
//	MSFilter *write_resampler;
//	MSFilter *equalizer;
//	MSFilter *dummy;
	uint64_t last_packet_count;
	time_t last_packet_time;
//	EchoLimiterType el_type; /*use echo limiter: two MSVolume, measured input level controlling local output level*/
//	MSQualityIndicator *qi;
	time_t start_time;
	uint32_t features;
	bool_t play_dtmfs;
	bool_t use_gc;
	bool_t use_agc;
	bool_t eq_active;
	bool_t use_ng;/*noise gate*/
	bool_t is_beginning;
};
/**
 * The AudioStream holds all resources to create and run typical VoIP audiostream.
 **/
typedef struct _AudioStream AudioStream;


typedef enum _VideoStreamDir{
	VideoStreamSendRecv,
	VideoStreamSendOnly,
	VideoStreamRecvOnly
}VideoStreamDir;

struct _VideoStream
{
	MediaStream ms;
//	MSFilter *source;
//	MSFilter *pixconv;
//	MSFilter *sizeconv;
//	MSFilter *tee;
//	MSFilter *output;
//	MSFilter *tee2;
//	MSFilter *jpegwriter;
//	MSFilter *output2;
//	MSVideoSize sent_vsize;
	int corner; /*for selfview*/
//	VideoStreamRenderCallback rendercb;
	void *render_pointer;
//	VideoStreamEventCallback eventcb;
	void *event_pointer;
	char *display_name;
	unsigned long window_id;
	unsigned long preview_window_id;
	VideoStreamDir dir;
//	MSWebCam *cam;
	int device_orientation; /* warning: meaning of this variable depends on the platform (Android, iOS, ...) */
	bool_t use_preview_window;
	bool_t display_filter_auto_rotate_enabled;
	bool_t prepare_ongoing;
};

typedef struct _VideoStream VideoStream;

#define LINPHONE_CALL_STATS_AUDIO 0
#define LINPHONE_CALL_STATS_VIDEO 1
/**
 * Enum describing ICE states.
 * @ingroup initializing
 **/
enum _SerPhoneIceState{
	SerPhoneIceStateNotActivated, /**< ICE has not been activated for this call */
	SerPhoneIceStateFailed, /**< ICE processing has failed */
	SerPhoneIceStateInProgress, /**< ICE process is in progress */
	SerPhoneIceStateHostConnection, /**< ICE has established a direct connection to the remote host */
	SerPhoneIceStateReflexiveConnection, /**< ICE has established a connection to the remote host through one or several NATs */
	SerPhoneIceStateRelayConnection /**< ICE has established a connection through a relay */
};

/**
 * Enum describing Ice states.
 * @ingroup initializing
 **/
typedef enum _SerPhoneIceState SerPhoneIceState;

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #CallStatsUpdated callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
 **/
typedef struct _SerPhoneCallStats SerPhoneCallStats;

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #CallStatsUpdated callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
 **/
struct _SerPhoneCallStats {
	int		type; /**< Can be either LINPHONE_CALL_STATS_AUDIO or LINPHONE_CALL_STATS_VIDEO*/
	jitter_stats_t	jitter_stats; /**<jitter buffer statistics, see oRTP documentation for details */
	mblk_t*		received_rtcp; /**<Last RTCP packet received, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	mblk_t*		sent_rtcp;/**<Last RTCP packet sent, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	float		round_trip_delay; /**<Round trip propagation time in seconds if known, -1 if unknown.*/
	SerPhoneIceState	ice_state; /**< State of ICE processing. */
//	LinphoneUpnpState	upnp_state; /**< State of uPnP processing. */
	float download_bandwidth; /**<Download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	float upload_bandwidth; /**<Download bandwidth measurement of sent stream, expressed in kbit/s, including IP/UDP/RTP headers*/
};


const char *serphone_global_state_to_string(SerphoneGlobalState gs);

#endif
