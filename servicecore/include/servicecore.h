#ifndef SERVICECORE_H_
#define SERVICECORE_H_
#include "salpr.h"
#include "sometools.h"
#include "serprivate.h"
#include "serphonecall.h"
#include "friends.h"
#include "lpconfig.h"
#include "sdk_common.h"
#include "critical_section_wrapper.h"

//#include "thread_wrapper.h"
#include "StunMessageCallBack.h"
#include "common_types.h"

#include "MediaStatisticsData.pb.h"

//#ifdef VIDEO_ENABLED
//#include "vie_capture.h"
//#include "vie_network.h"
//#include "send_statistics_proxy.h"
//#include "receive_statistics_proxy.h"
//
//#ifdef WIN32
//#include "chromakeyfilter.h"
//#endif
//#endif

typedef enum {
	Video_Conference_status_RequestTimeout = -1,
	Video_Conference_status_OK = 0,
	Video_Conference_status_NotExist,
	Video_Conference_status_UserExclusive,
	Video_Conference_status_RequestedUserExclusive,
	Video_Conference_status_RequestedUserNoVideo
} Video_Conference_status;

typedef enum {
	Video_Conference_State_Nomal =0,
	Video_Conference_State_Requesting,
	Video_Conference_State_RequestFailed,
	Video_Conference_State_Streaming,
	Video_Conference_State_Canceling,
} Video_Conference_State;


typedef struct {
	char *remoteSip;
	int local_port;
	void *video_window;
	bool is_waiting;
	time_t request_time;
	int request_status;
	int conference_state;
	int server_port;
} VideoConferenceDesc;



//#ifdef __APPLE__
//#define __int64 __int64_t
//#endif

#include <ortp_srtp.h>
#include <map>
#include <string>
/**Call state notification callback prototype*/
typedef void (*SerphoneGlobalStateCb)(class ServiceCore *lc, SerphoneGlobalState gstate, const char *message);
/**Call state notification callback prototype*/
typedef void (*SerphoneCallStateCb)(class ServiceCore *lc, SerPhoneCall *call, SerphoneCallState cstate, const char *message);
/**Call encryption changed callback prototype*/
typedef void (*CallEncryptionChangedCb)(class ServiceCore *lc, SerPhoneCall *call, bool_t on, const char *authentication_token);

/** @ingroup Proxies
 * Registration state notification callback prototype
 * */
typedef void (*SerphoneRegistrationStateCb)(class ServiceCore *lc, SerphoneProxyConfig *cfg, SerphoneRegistrationState cstate, const char *message);
/** Callback prototype */
typedef void (*ShowInterfaceCb)(class ServiceCore *lc);
/** Callback prototype */
typedef void (*DisplayStatusCb)(class ServiceCore *lc, const char *message);
/** Callback prototype */
typedef void (*DisplayMessageCb)(class ServiceCore *lc, const char *message);
/** Callback prototype */
typedef void (*DisplayUrlCb)(class ServiceCore *lc, const char *message, const char *url);
/** Callback prototype */
typedef void (*SerphoneCoreCbFunc)(class ServiceCore *lc,void * user_data);
/** Callback prototype */
typedef void (*NotifyReceivedCb)(class ServiceCore *lc, SerPhoneCall *call, const char *from, const char *event);
/** Media initialize failed callback prototype 0:audio 1:video*/
typedef void (*MediaInitFailed)(class ServiceCore *lc, SerPhoneCall *call, int type, int error);
/** Deliver local video frame in RGB24 druing video call*/
typedef void (*DeliverVideoFrame)(class ServiceCore *lc, SerPhoneCall *call, uint8_t* buf, int len, int width, int height);
typedef void (*RecordAudioCb)(class ServiceCore *lc, SerPhoneCall *call, const char *file_name, int status);
typedef void (*ThrowDataToProcessCb)(class ServiceCore *lc, SerPhoneCall *call, const void *inData, int inLen, void *outData, int &outLen, bool send);

typedef void (*ConnectCameraFailed)(class ServiceCore *lc, SerPhoneCall *call, int cameraIndex, const char *cameraName);

typedef void (*ThrowOriginalDataToProcessCb)(class ServiceCore *lc, SerPhoneCall *call, const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send);
typedef void (*RequestSpecifiedVideoFailed)(class ServiceCore *lc, SerPhoneCall *call, const char *sip, int reason);
typedef void (*StopSpecifiedVideoResponse)(class ServiceCore *lc, SerPhoneCall *call, const char *sip, int response, void *window);
typedef void (*RemoteVideoRatioChanged)(SerPhoneCall *call, int width, int height,bool isVideoConference, const char *sipNo);
typedef void (*InitHaiyuntongFailed)();
typedef void (*EXosipThreadStop)();

typedef void (*VideoCaptureStatus)(class ServiceCore *lc, SerPhoneCall *call, int status);
typedef void (*VideoPacketTimeOut)(class ServiceCore *lc, SerPhoneCall *call, int type);

/**
 * Report status change for a friend previously \link linphone_core_add_friend() added \endlink to #LinphoneCore.
 * @param lc #ServiceCore object .
 * @param lf Updated #LinphoneFriend .
 */
typedef void (*NotifyPresenceReceivedCb)(class ServiceCore *lc, SerphoneFriend * lf);
/**
 *  Reports that a new subscription request has been received and wait for a decision.
 *  <br> Status on this subscription request is notified by \link serphone_friend_set_inc_subscribe_policy() changing policy \endlink for this friend
 *	@param lc #ServiceCore object
 *	@param lf #SerphoneFriend corresponding to the subscriber
 *	@param url of the subscriber
 *  Callback prototype
 *  */
typedef void (*NewSubscribtionRequestCb)(class ServiceCore *lc, SerphoneFriend *lf, const char *url);
/** Callback prototype */
typedef void (*AuthInfoRequested)(class ServiceCore *lc, const char *realm, const char *username);
/** Callback prototype */
typedef void (*CallLogUpdated)(class ServiceCore *lc, SerphoneCallLog *newcl);
/**
 * Callback prototype
 *
 * @param room #SerphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param from #SerphoneAddress from
 * @param message incoming message
 *  */
typedef void (*TextMessageReceived)(class ServiceCore *lc, SerphoneChatRoom *room, const SerphoneAddress *from, const SerphoneAddress *to, const char *msgid, const char *message, const char *date);
typedef void (*TextSendReport)(class ServiceCore *lc, const char *msgid, const char *date,  int result);
/** Callback prototype */
typedef void (*DtmfReceived)(class ServiceCore* lc, const char *callid, char dtmf);
/** Callback prototype */
typedef void (*ReferReceived)(class ServiceCore *lc, const char *refer_to);
/** Callback prototype */
typedef void (*BuddyInfoUpdated)(class ServiceCore *lc, SerphoneFriend *lf);
/** Callback prototype for in progress transfers. The new_call_state is the state of the call resulting of the transfer, at the other party. */
typedef void (*LinphoneTransferStateChanged)(class ServiceCore *lc, SerPhoneCall *transfered, SerphoneCallState new_call_state);


typedef void (*ReceiverStatsReceived)(class ServiceCore *lc, const char *callid, const int framerate, const int bitrate);
typedef void (*ReceiverCodecChanged)(class ServiceCore *lc, const char *callid, const int width, const int height);
/**
 * This structure holds all callbacks that the application should implement.
 *  None is mandatory.
**/
typedef struct _SerphoneVTable{
	SerphoneGlobalStateCb global_state_changed; /**<Notifies globlal state changes*/
	SerphoneRegistrationStateCb registration_state_changed;/**<Notifies registration state changes*/
	SerphoneCallStateCb call_state_changed;/**<Notifies call state changes*/
	NotifyPresenceReceivedCb notify_presence_recv; /**< Notify received presence events*/
	NewSubscribtionRequestCb new_subscription_request; /**< Notify about pending subscription request */
	AuthInfoRequested auth_info_requested; /**< Ask the application some authentication information */
	CallLogUpdated call_log_updated; /**< Notifies that call log list has been updated */
	TextMessageReceived text_received; /**< A text message has been received */
	TextSendReport text_send_report;
	DtmfReceived dtmf_received; /**< A dtmf has been received received */
	ReferReceived refer_received; /**< An out of call refer was received */
	CallEncryptionChangedCb call_encryption_changed; /**<Notifies on change in the encryption of call streams */
    LinphoneTransferStateChanged transfer_state_changed; /**<Notifies when a transfer is in progress */
	BuddyInfoUpdated buddy_info_updated; /**< a LinphoneFriend's BuddyInfo has changed*/
	NotifyReceivedCb notify_recv; /**< Other notifications*/
	DisplayStatusCb display_status; /**< Callback that notifies various events with human readable text.*/
	DisplayMessageCb display_message;/**< Callback to display a message to the user */
	DisplayMessageCb display_warning;/** Callback to display a warning to the user */
	DisplayUrlCb display_url;
	ShowInterfaceCb show; /**< Notifies the application that it should show up*/
    MediaInitFailed media_init_failed; /**  Notifies the application that media initialize failed*/
    DeliverVideoFrame deliver_video_frame;
    RecordAudioCb record_audio_callback;
    ThrowDataToProcessCb throw_data_2_process;
	ConnectCameraFailed connect_camera_failed;
    ThrowOriginalDataToProcessCb throw_original_data_2_process;
    RequestSpecifiedVideoFailed request_specified_video_failed;
    StopSpecifiedVideoResponse stop_specified_video_response;
    RemoteVideoRatioChanged remote_video_ratio_changed;
    InitHaiyuntongFailed init_haiyuntong_failed;
    EXosipThreadStop eXosip_thread_stop;
	VideoCaptureStatus video_capture_status;
	VideoPacketTimeOut video_packet_timeout;
	ReceiverStatsReceived receiver_stats_received;
	ReceiverCodecChanged  receiver_codec_changed;
} SerphoneCoreVTable;

namespace cloopenwebrtc{
class VoiceEngine;
class VideoEngine;
class SendStatisticsProxy;
}

class ServiceCore : public cloopenwebrtc::ServiceCoreCallBack
#ifdef VIDEO_ENABLED
//	, public cloopenwebrtc::ViECaptureObserver
//	, public cloopenwebrtc::ViENetworkObserver
#endif
{
public:
	ServiceCore();
	~ServiceCore();
public:
	void serphone_core_init (const SerphoneCoreVTable *vtable, const char *config_path,
              const char *factory_config_path, void * userdata);
    void serphone_core_uninit();
	void serphone_core_iterate();
private:
    bool tempAuth;
public:
    void serphone_core_enable_temp_auth(bool flag);
    bool serphone_core_get_temp_auth();
    const char *serphone_core_get_registerUserdata();
    const char * serphone_core_get_identity();
    const char *serphone_core_get_nat_address_resolved();
	void serphone_core_set_guess_hostname( bool_t val);
	bool_t serphone_core_get_guess_hostname();

	void serphone_set_reg_info(const char *proxy_addr, int proxy_port,
		const char *account, const char *password, const char *displayname, const char *capability_token, const char *gTokenp);
    void serphone_proxy_remove(const char *proxy_addr);
	void serphone_set_reg_displayname(const char*displayname);
	void apply_user_agent(const char * agent=NULL);
//////////proxy  route
    SerphoneProxyConfig * serphone_core_lookup_known_proxy(const SerphoneAddress *uri);
    const char *serphone_core_find_best_identity(const SerphoneAddress *to, const char **route);
/////////////////call
    SerPhoneCall * serphone_call_new_outgoing(SerphoneAddress *from, SerphoneAddress *to,
		const SerphoneCallParams *params);
	SerPhoneCall * serphone_call_new_incoming(SerphoneAddress *from, SerphoneAddress *to, SalOp *op);
	bool_t already_a_call_pending();

	bool_t already_a_call_with_remote_address(const SerphoneAddress *remote);

    bool_t serphone_core_can_we_add_call();
	SerPhoneCall* serphone_core_find_call_by_cid( int cid);
	int serphone_core_add_call(SerPhoneCall *call);
	int serphone_core_del_call(SerPhoneCall *call);
	int serphone_core_accept_call(SerPhoneCall *call);
    int serphone_core_terminate_call(SerPhoneCall *call);
    int serphone_core_terminate_all_calls();
	int serphone_core_abort_call(SerPhoneCall *call, const char *error);
	int serphone_core_accept_call_update(SerPhoneCall *call, const SerphoneCallParams *params);
    int serphone_core_pause_call(SerPhoneCall *call);

    int serphone_core_pause_all_calls();

    int serphone_core_resume_call(SerPhoneCall *call);

    int serphone_core_update_call(SerPhoneCall *call, const SerphoneCallParams *params);

    int serphone_core_defer_call_update(SerPhoneCall *call);
	///////////////invite
    SerphoneAddress * serphone_core_interpret_url(const char *url);
    SerPhoneCall * serphone_core_invite( const char *url,char *userdata);
    SerPhoneCall * serphone_core_invite_address(const SerphoneAddress *addr);
    SerPhoneCall * serphone_core_invite_with_params(const char *url, const SerphoneCallParams *params);
    SerPhoneCall * serphone_core_invite_address_with_params(const SerphoneAddress *addr, const SerphoneCallParams *params);
    int serphone_core_transfer_call( SerPhoneCall *call, const char *refer_to, int type);
    int serphone_core_transfer_call_to_another(SerPhoneCall *call, SerPhoneCall *dest);
    bool_t serphone_core_inc_invite_pending();
    bool_t serphone_core_in_call();
    int serphone_core_accept_call_with_params(SerPhoneCall *call, const SerphoneCallParams *params);
    int serphone_core_start_invite(SerPhoneCall *call, SerphoneProxyConfig *dest_proxy);
    void serphone_core_start_refered_call(SerPhoneCall *call);

////////////////////////presence//////////////
    void serphone_core_add_subscriber(const char *subscriber, SalOp *op);
    void serphone_core_send_initial_subscribes();
	void serphone_subscription_new(SalOp *op, const char *from);
    void serphone_notify_recv(SalOp *op, SalSubscribeStatus ss, SalPresenceStatus status);
    void serphone_subscription_closed(SalOp *op);
	void serphone_core_reject_subscriber(SerphoneFriend *lf);
    void serphone_core_notify_all_friends(SerphoneOnlineStatus os);
	void serphone_core_add_friend(SerphoneFriend *lf);
    void serphone_core_notify_refer_state(SerPhoneCall *referer, SerPhoneCall *newcall);

    void serphone_core_text_received(const char *from, const char *to, const char *msgid, const char *msg,const char* date);
	const char *serphone_core_send_text(const char*to, const char*msg);
    SerphoneChatRoom * serphone_core_create_chat_room(const char *to);

	int find_port_offset();

    int serphone_core_run_stun_tests(SerPhoneCall *call);

	int serphone_core_check_account_online(char *url);

///////////////////param
    SerphoneCallParams *serphone_core_create_default_call_parameters();

//////////////////////////authen
    void serphone_core_add_auth_info(const SerphoneAuthInfo *info);
    void serphone_core_remove_auth_info(const SerphoneAuthInfo *info);
    const SerphoneAuthInfo *serphone_core_find_auth_info(const char *realm, const char *username);
    void serphone_core_abort_authentication( SerphoneAuthInfo *info);
    void serphone_core_clear_all_auth_info();
	void serphone_core_update_auth_info(const char *realm, const char *username);

/////////////////////write config
    void serphone_core_write_friends_config();

///////////////////attribute
    SerphoneGlobalState serphone_core_get_global_state();

    const MSList *serphone_core_get_auth_info_list();
	void serphone_core_verify_server_certificates(bool_t yesno);

    SerphoneMediaEncryption serphone_core_get_media_encryption();

    SerphoneFriend *serphone_core_get_friend_by_address(const char *addr);
    SerphoneFriend *serphone_core_get_friend_by_ref_key(const char *key);
	const MSList * serphone_core_get_friend_list();

    void serphone_core_set_download_bandwidth(int bw);
    void serphone_core_set_upload_bandwidth(int bw);
	int serphone_core_get_download_bandwidth();
    int serphone_core_get_upload_bandwidth();
	double get_audio_payload_bandwidth(const PayloadType *pt);

    int serphone_core_get_audio_port();
    int serphone_core_get_video_port();
    void serphone_core_set_audio_port(int port);
    void serphone_core_set_video_port(int port);

	void serphone_core_set_stun_server(const char *server);
    const char * serphone_core_get_stun_server();

	int serphone_core_get_current_call_duration();
    SerPhoneCall *serphone_core_get_current_call();
    const MSList *serphone_core_get_calls();
	SerPhoneCall *serphone_core_get_call_by_remote_address(const char *remote_address);

    SerphoneAddress *serphone_core_get_primary_contact_parsed();
	const char *serphone_core_get_primary_contact();
	int serphone_core_set_primary_contact(const char *contact);
	void update_primary_contact();
	void serphone_core_get_local_ip(const char *to, char *result);
	int serphone_core_set_sip_transports(const LCSipTransports * tr);
	int serphone_core_get_sip_transports(LCSipTransports *tr);
	int serphone_core_get_sip_port();
	bool_t serphone_core_get_use_info_for_dtmf();
	void serphone_core_set_use_info_for_dtmf(bool_t use_info);
	bool_t serphone_core_get_use_rfc2833_for_dtmf();
	void serphone_core_set_use_rfc2833_for_dtmf(bool_t use_rfc2833);
    void serphone_core_enable_ipv6(bool_t val);
	bool_t serphone_core_ipv6_enabled();
	ortp_socket_t serphone_core_get_sip_socket();

    const char * serphone_core_get_route();
	MSList *serphone_core_get_proxy_config_list();
    MSList *serphone_core_get_remove_config_list();
    int serphone_core_get_default_proxy(SerphoneProxyConfig **config);
	bool_t serphone_proxy_config_check( SerphoneProxyConfig *obj);
	void serphone_proxy_config_write_all_to_config_file();
	int serphone_core_add_proxy_config(SerphoneProxyConfig *cfg);
	void serphone_core_set_default_proxy_index(int index);
	void serphone_core_set_default_proxy(SerphoneProxyConfig *config);

    void serphone_core_set_inc_timeout(int seconds);
    int serphone_core_get_inc_timeout();
	int serphone_core_get_upload_ptime();
    void serphone_core_set_nat_address( const char *addr);
    const char *serphone_core_get_nat_address();
    void serphone_core_set_firewall_policy( SerphoneFirewallPolicy pol);
    SerphoneFirewallPolicy serphone_core_get_firewall_policy();
    const char * serphone_core_get_relay_addr();
    int serphone_core_set_relay_addr(const char *addr);

    void serphone_core_set_audio_jittcomp( int value);
	void serphone_core_set_rtp_no_xmit_on_audio_mute(bool_t rtp_no_xmit_on_audio_mute);
	bool_t serphone_core_get_rtp_no_xmit_on_audio_mute( );
	void serphone_core_set_nortp_timeout(int nortp_timeout);
	int  serphone_core_get_nortp_timeout();
    void serphone_core_set_remote_ringback_tone(const char *);
    const char *serphone_core_get_remote_ringback_tone();

	bool_t serphone_core_adaptive_rate_control_enabled();

	void serphone_core_restart_nack(SerPhoneCall *call);

//////////////////////media deal
	void ring_stop(int ringmode = 0);
	int  ring_start(const char *file, int interval, int ringmode=0);
	void serphone_core_play_tone();
    void serphone_core_update_streams(SerPhoneCall *call, SalMediaDescription *new_md);
	void serphone_call_stop_media_streams(SerPhoneCall *call);
	void serphone_call_start_media_streams(SerPhoneCall *call, bool_t all_inputs_muted, bool_t send_ringbacktone);

    void serphone_core_start_dtmf_stream();
    void serphone_core_stop_dtmf_stream();

    void serphone_core_stop_dtmf( );

    void serphone_call_enable_camera(SerPhoneCall *lc, bool_t enabled);
    bool_t serphone_call_camera_enabled(const SerPhoneCall *lc);

    void serphone_core_mute_mic(bool_t muted);
    bool_t serphone_core_is_mic_muted();
//the following function for IOS,any question???
	void audio_stream_prepare_sound(SerPhoneCall *call);
	void audio_stream_unprepare_sound(SerPhoneCall *call);
    void video_stream_prepare_video(VideoStream *stream);
    void video_stream_unprepare_video(VideoStream *stream);
///end

	void audio_stream_play(SerPhoneCall *call, const char *name);
	const MSList *serphone_core_get_audio_codecs();

	void serphone_core_send_dtmf(char dtmf);
	void serphone_core_playfile_to_remote(SerPhoneCall *call,char * filename);
	void serphone_core_stop_playfile_to_remote(SerPhoneCall *call);

    int serphone_set_louds_speaker_status(bool bLouds);
    int serphone_get_louds_speaker_status();
	int serphone_set_mute_status(bool bLouds);
	int serphone_get_mute_status();

	int serphone_set_speaker_mute_status(bool bLouds);
	int serphone_get_speaker_mute_status();

	void serphone_core_enable_payload_type(const char *mimeType, int freq, bool bEnable);
	bool serphone_core_is_payload_type_enable(const char *mimeType, int freq);

	void serphone_core_enable_srtp(bool tls, bool srtp, bool userMode, int cryptType, const char *pkey);

private:
    int local_playfile_channelID_prering;
    int local_playfile_channelID_afterring;
public:
    void serphone_core_set_ring(const char *path);
	void serphone_core_set_ringback( const char *path);
    void serphone_core_set_prering(const char *path);
    void serphone_core_set_afterring( const char *path);
    int serphone_prering_start();
    int serphone_afterring_start();
    void serphone_pre_after_ring_stop(bool ispreRing);
    void serphone_check_pre_after_ring_timeout();
    virtual void onStopPlayPreRing();


	int selectCamera(int cameraIndex, int capabilityIndex,int fps,int rotate,bool force);
	int getCameraInfo(CameraInfo **);
	int getShareScreenInfo(ScreenID **screenId, int captureId);
	int getShareWindowInfo(WindowShare **windowInfo, int captureId);

	int getCallStatistics(int type, MediaStatisticsInfo *);

    static int return_video_width_height(int width,int height, int videoChannelID);
	static int onLiveStreamVideoResolution(void *handle, int width, int height);

	void serphone_core_set_audio_pacinterval(int pacsize);
	int serphone_core_get_audio_pacinterval();

	int serphone_core_set_audio_config_enabled(int type, bool_t enabled, int mode);
	int serphone_core_get_audio_config_enabled(int type, bool_t *enabled, int *mode);

    int serphone_core_reset_audio_device();
    void serphone_core_set_dtx_enabled(bool_t enabled);

    int serphone_core_start_rtp_dump(SerPhoneCall *call, int mediatype, const char* file, cloopenwebrtc::RTPDirections direction);
    int serphone_core_stop_rtp_dump(SerPhoneCall *call, int mediatype, cloopenwebrtc::RTPDirections direction);
	int serphone_core_set_speaker_volume(unsigned int volume);
	unsigned int serphone_core_get_speaker_volume();

    int setOutVolumeScaling(SerPhoneCall *call, float scaling);

    //ICE
    //sean ice
    void serphone_core_update_ice_from_remote_media_description(SerPhoneCall *call, const SalMediaDescription *md);
    bool_t serphone_core_media_description_contains_video_stream(const SalMediaDescription *md);
    void serphone_call_start_media_streams_for_ice_gathering(SerPhoneCall *call);
    void serphone_call_stop_media_streams_for_ice_gathering(SerPhoneCall *call);
    int serphone_core_gather_ice_candidates(/*SerphoneCore *lc, */SerPhoneCall *call);
    virtual  void onStunPacket(const char*call_id,void*data,int len,const char *fromIP ,int fromPort, bool isRTCP = 0, bool isVideo = 0);
    virtual void  onAudioData(const char *call_id, const void *inData, int inLen, void *outData, int &outLen, bool send);
    virtual void onOriginalAudioData(const char *call_id, const void *inData, int inLen, int sampleRate, int numChannels, bool send);

    void serphone_core_set_video_bitrates(int bitrates);
    void serphone_core_reg_kickedoff();
    SerPhoneCall* serphone_core_find_call_by_user_cid(const char* cid);
    /*add begin------------------Sean20130729----------for video ice------------*/
//    const char *get_user_call_id();
//    void set_user_call_id(const char * user_call_id);

//sean add begin 0915
    void serphone_set_mosaic(bool flag);
    void serphone_set_rate_p2p(int rate);
//sean add end 0915

    //audio record
	int serphone_call_start_record_audio(SerPhoneCall *call, const char *filename);
	int serphone_call_stop_record_audio(SerPhoneCall *call);
    int record_audio_status(SerPhoneCall *call, const char *filename, int status);
	int serphone_call_start_record_audio_ex(SerPhoneCall *call, const char *rFileName, const char *lFileName);

	int serphone_call_start_record_r_video(SerPhoneCall *call, const char *filename);
	int serphone_call_stop_record_r_video(SerPhoneCall *call);

	int serphone_call_start_record_l_video(SerPhoneCall *call, const char *filename);
	int serphone_call_stop_record_l_video(SerPhoneCall *call);

	/*   int serphone_call_start_record_voip(SerPhoneCall *call, const char *filename);
	int serphone_call_stop_record_voip(SerPhoneCall *call);
	int record_voip_status(SerPhoneCall *call, const char *filename, int status);*/

	int serphone_call_start_record_screen_ex(SerPhoneCall *call, const char *filename, int bitrates, int fps, int type, int left, int top, int width, int height);
	int serphone_call_start_record_screen(SerPhoneCall *call, const char *filename, int bitrates, int fps, int type);
	int serphone_call_stop_record_screen(SerPhoneCall *call);

	int serphone_call_get_network_statistic(SerPhoneCall *call,  long long *duration, long long *sendTotalSim, long long *recvTotalSim, long long *sendTotalWifi, long long *recvTotalWifi);


	int startSendRtpPacket(int &channel, const char *ip, int rtp_port);
	int startRecvRtpPacket(int channelNum);
	int serphone_audio_set_magic_sound(SerPhoneCall *call, int pitch, int tempo, int rate);
	int serphone_audio_enable_magic_sound(SerPhoneCall *call, bool is_enable);

private:
//    char _user_call_id[9];
//    sean add begin 0915
    bool _shield_mosaic;
    int  _rate_after_p2p;
//    sean add end 0915
public:
    int _terminate_call;
    /*add end--------------------Sean20130729----------for video ice------------*/

	int getPlayoutDeviceInfo(SpeakerInfo** speakerinfo);
	int selectPlayoutDevice(int index);

	int getRecordDeviceInfo(MicroPhoneInfo** microphoneinfo);
	int selectRecordDevice(int index);
public:
//    SerPhoneCall* serphone_core_find_call_by_audio_channel_id( int channel_id);
//    SerPhoneCall* serphone_core_find_call_by_video_channel_id( int channel_id);
    int startDeliverVideoFrame(SerPhoneCall *call);
    int stopDeliverVideoFrame(SerPhoneCall *call);
    void DeliverFrameData(SerPhoneCall *call, unsigned char*buf, int size, int width, int height);

    int getLocalVideoSnapshot(SerPhoneCall *call, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
	int getLocalVideoSnapshot(SerPhoneCall *call, const char* filePath);
    int getRemoteVideoSnapshot(SerPhoneCall *call, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
	int getRemoteVideoSnapshot(SerPhoneCall *call, const char* filePath);

	int startVideoWithoutCall(int cameraIndex, int videoW, int videoH, int rotate, void *videoWnd);
	int stopVideoWithoutCall();
	int getSnapshotWithoutCall(const char *filePath);

	int serphone_call_reset_video_views(SerPhoneCall *call, void* remoteView,void *localView);
	int startVideoCapture(SerPhoneCall *call);
	int startVideoDesktopCapture(SerPhoneCall *call);

	int PlayAudioFromRtpDump(int localPort, const char *ptName, int ploadType,
		cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key);
	int StopPlayAudioFromRtpDump();

	int PlayVideoFromRtpDump(int localPort, const char *ptName, int ploadType, void *videoWindow,
		cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key);
	int StopPlayVideoFromRtpDump();
	void SetNackEnabled(bool audioEnabled, bool videoEnabled);
	void SetVideoProtectionMode(int mode);

	void SetP2PEnabled(bool enable);
	void SetRembEnabled(bool enable);
	void SetTmmbrEnabled(bool enable);

	//void setVideoCode(int videoCodeIndex); //only for demo test 0: H264, 1:vp8
	void setVideoMode(int videoModeIndex); //only for demo test 0: Real-time, 1:screen-share
	void setDesktopShareParam(int desktop_width, int desktop_height, int desktop_frame_rate, int desktop_bit_rate);

   int setScreeShareActivity(SerPhoneCall *call, void *activity);
    int startRecord();
    int stopRecord();
protected:
///////////////param
	void serphone_core_init_default_params(SerphoneCallParams *params);
	SalMediaDescription *create_local_media_description( SerPhoneCall *call);
	SalMediaDescription *_create_local_media_description( SerPhoneCall *call, unsigned int session_id, unsigned int session_ver);
	void update_local_media_description(SerPhoneCall *call);
	int serphone_core_get_calls_nb();
	void discover_mtu(const char *remote);
	bool serphone_core_ready(){
		return state!=LinphoneGlobalStartup;
	}
	void terminate_call(SerPhoneCall *call);
	char *get_fixed_contact(SerPhoneCall *call , SerphoneProxyConfig *dest_proxy);
	void serphone_core_preempt_sound_resources();
///////////authen
	void write_auth_infos();
	//set servicecore state
	void serphone_core_set_state(SerphoneGlobalState gstate, const char *message);
	void serphone_core_assign_payload_type( PayloadType *const_pt, int number, const char *recv_fmtp);
	bool_t serphone_core_payload_type_enabled(const PayloadType *pt);
	void serphone_core_free_payload_types();
	void serphone_core_handle_static_payloads();
	int apply_transports();
	void transport_error(const char* transport, int port);
	void __serphone_core_invalidate_registers();
	void serphone_core_run_hooks();
	void serphone_core_free_hooks();
	void proxy_update();
	void monitor_network_state(time_t curtime);
	void set_network_reachable(bool_t isReachable, time_t curtime);
	bool_t serphone_core_is_payload_type_usable_for_bandwidth(PayloadType *pt,  int bandwidth_limit);
	bool_t serphone_core_check_payload_type_usability(PayloadType *pt);
	bool_t serphone_core_video_enabled();
	void serphone_core_update_allocated_audio_bandwidth();
	int serphone_core_set_audio_codecs(MSList *codecs);
	int serphone_core_set_video_codecs(MSList *codecs);
	bool_t get_codec(LpConfig *config, const char* type, int index, PayloadType **ret);

	void serphone_call_init_media_streams(SerPhoneCall *call);
	void serphone_call_start_audio_stream(SerPhoneCall *call, const char *cname, bool_t muted, bool_t send_ringbacktone, bool_t use_arc);
	void serphone_call_start_video_stream(SerPhoneCall *call, const char *cname,bool_t all_inputs_muted);
	void serserphone_call_start_desktop_share(SerPhoneCall *call, const char *cname,bool_t all_inputs_muted);
    void media_stream_free(MediaStream *stream);
    void audio_stream_free(AudioStream *stream);
    void audio_stream_stop(int channelID);
    void video_stream_free(VideoStream *stream);
	void video_stream_stop(int channelID, int captureID);

public:
    //void media_init_audio();
    //void media_uninit_audio();
private:
    //void media_init_video();
    //void media_uninit_video();
	void rtp_config_read();
	void rtp_config_uninit();

	void sound_config_read();
    void sound_config_uninit();
	void sip_config_read();
public:
    int sip_check_thread_active();
private:
	void sip_config_uninit();
	void ui_config_read();
	void ui_config_uninit();
	void misc_config_read ();
	void net_config_read ();
	void net_config_uninit();
	void codecs_config_read();
	void codecs_config_uninit();
    void video_config_read();

    void serphone_core_set_video_policy(const SerphoneVideoPolicy *policy);

	MSList *make_codec_list(const MSList *codecs, int bandwidth_limit,int* max_sample_rate);
	int get_codec_bitrate(const PayloadType *pt);


    static bool_t generate_b64_crypto_key(int key_length, char* key_out, const char *user_key);


    //sean ice
//    void sean_dump_sal_ice_candidates(SalIceCandidate *first, int size);
//    void sean_dump_ice_candidate(IceCandidate *candidate);
    int sendStunRequest(int sock, const struct sockaddr *server, socklen_t addrlen, int id, bool_t changeAddr);
//    int recvStunResponse(ortp_socket_t sock, char *ipaddr, int *port, int *id, SerPhoneCall *call);
    int recvStunResponse(ortp_socket_t sock, char *ipaddr, int *port, int *id);
    //Handle kinds of response, including stun response ,stun request , etc.
    void get_default_addr_and_port(uint16_t componentID, const SalMediaDescription *md, const SalStreamDescription *stream, const char **addr, int *port);
//    void serphone_call_delete_ice_session(SerPhoneCall *call);

    void serphone_call_background_tasks(SerPhoneCall *call, bool_t one_second_elapsed);
    void video_stream_iterate(VideoStream *stream);
    void audio_stream_iterate(AudioStream *stream);
public:
    void serphone_call_delete_ice_session(SerPhoneCall *call);
    void handle_ice_events(SerPhoneCall *call, OrtpEvent *ev, void *data, bool isVideo = 0);
private:
    void update_media_description_from_stun(SalMediaDescription *md, const StunCandidate *ac, const StunCandidate *vc);
    void serphone_core_update_ice_state_in_call_stats(SerPhoneCall *call);
    int serphone_core_start_update_call(SerPhoneCall *call);

public:
    void serphone_core_update_local_media_description_from_ice(SalMediaDescription *desc, IceSession *session);
    void serphone_call_update_remote_session_id_and_ver(SerPhoneCall *call);
public:
    int serphone_core_start_accept_call_update(SerPhoneCall *call);
    int serphone_core_proceed_with_invite_if_ready(SerPhoneCall *call, SerphoneProxyConfig *dest_proxy);
    void serphone_core_update_streams_destinations(SerPhoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md);
    void serphone_call_update_crypto_parameters(SerPhoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md);
public:
    void serphone_core_notify_incoming_call(SerPhoneCall *call);
private:
    AudioStream *audio_stream_new(int loc_rtp_port, int loc_rtcp_port, bool_t ipv6, int channel);
    VideoStream *video_stream_new(int loc_rtp_port, int loc_rtcp_port, bool_t use_ipv6, int channel);
    RtpSession * create_duplex_rtpsession(int loc_rtp_port, int loc_rtcp_port, bool_t ipv6,int isVideo = 1);
    bool_t serphone_core_incompatible_security(SalMediaDescription *md);
    bool_t serphone_core_is_media_encryption_mandatory();
    void serphone_call_init_audio_stream(SerPhoneCall *call);
    void serphone_call_init_video_stream(SerPhoneCall *call);
    int serphone_core_get_audio_dscp(const ServiceCore *lc);
    int audio_stream_set_dscp(AudioStream *stream, int dscp);
    int media_stream_set_dscp(MediaStream *stream, int dscp);
    const char * media_stream_type_str(MediaStream *stream);
    bool_t serphone_core_echo_limiter_enabled(const ServiceCore *lc);

    void serphone_core_parse_capability_token(const char *token);

public:
    bool_t serphone_core_set_process_audio_data_flag(SerPhoneCall *call,bool flag);
    bool_t serphone_core_set_process_original_audio_data_flag(SerPhoneCall *call,bool flag);
private:
    bool_t serphone_core_set_process_audio_data_flag_internel(SerPhoneCall *call);


public:
#ifdef VIDEO_ENABLED
	//void BrightnessAlarm(const int capture_id, const cloopenwebrtc::Brightness brightness);
	//void CapturedFrameRate(const int capture_id, const unsigned char frame_rate);
	//void NoPictureAlarm(const int capture_id, const cloopenwebrtc::CaptureAlarm alarm);

	// This method will be called periodically delivering a dead‐or‐alive
	// decision for a specified channel.
	//virtual void OnPeriodicDeadOrAlive(const int video_channel, const bool alive);
	// This method is called once if a packet timeout occurred.
	//virtual void PacketTimeout(const int video_channel, const cloopenwebrtc::ViEPacketTimeout type);
#endif

	int FixedCameraInfo(const char *cameraName, const char *cameraId, int width, int height);
	int ConfigureChromaKey(const char *bgImage, float angle, float noise_level, int r=-1, int g=-1, int b=-1);
	int StartVirtualBackGround();
	int StopVirtualBakcGround();

	int SetVideoKeepAlive(SerPhoneCall *call, bool enable, int interval);
	int SetAudioKeepAlive(SerPhoneCall *call, bool enable, int interval);

public:
	static SerphoneCoreVTable vtable;
	Sal    *sal;
	SerphoneGlobalState state;
	struct _LpConfig *config;
	net_config_t   net_conf;
	sip_config_t   sip_conf;
	rtp_config_t   rtp_conf;
	sound_config_t sound_conf;
	video_config_t video_conf;
	codecs_config_t codecs_conf;
    capability_config_t capability_conf;

	MSList *payload_types;
	int dyn_pt;
	SerphoneProxyConfig *default_proxy;
	MSList *friends;
	MSList *auth_info;
	SerPhoneCall * current_call;   /* the current call */
	MSList *calls;				  /* all the processed calls */
	MSList *call_logs;
	MSList *chatrooms;
	SerphoneOnlineStatus  presence_mode;
	char *alt_contact;
	void *data;
	time_t prevtime;
	int max_call_logs;
	int missed_calls;
	MSList *bl_reqs;
	MSList *subscribers;	/* unknown subscribers */

	int audio_bw;
	time_t netup_time; /*time when network went reachable */
	MSList *hooks;
	SerphoneVideoPolicy video_policy;
	bool_t initial_subscribes_sent;
	bool_t bl_refresh;
	bool_t auto_net_state_mon;
	bool_t network_reachable;
	bool_t ringstream_autorelease;
	int max_calls;
//////the following code added fromwebrtc
 //   cloopenwebrtc::VoiceEngine* m_voe;
	//static cloopenwebrtc::VideoEngine* m_vie;
    cloopenwebrtc::CriticalSectionWrapper *m_criticalSection;
	bool_t m_ringplay_flag;
	time_t dmfs_playing_start_time;
	int    local_playfile_channelID;
	void  *m_videoWindow;
    void  *localVideoWindow;
	int videoWindowSize;
	int localVideoWindowSize;
	CameraInfo *m_cameraInfo;
	int m_cameraCount;
	int m_usedCameraIndex;
	int m_usedCapabilityIndex;

	ScreenID		*m_pScreenInfo;
	WindowShare		*m_pWindowInfo;
	int m_desktopCaptureId;

	int m_maxFPS;
	int m_camerRotate;
	//MediaStatisticsInfo m_lastCallStatisticsInfo;

	int m_sendVideoWidth;
	int m_sendVideoHeight;
	int m_sendVideoFps;

	SpeakerInfo *m_speakerInfo;
	int m_speakerCount;
	MicroPhoneInfo *m_microphoneInfo;
	int m_microphoneCount;

	int m_usedSpeakerIndex;
	int m_usedMicrophoneIndex;

    int m_silkBitRate;
    int m_packetInterval;
    bool_t m_agcEnabled;
    bool_t m_ecEnabled;
    bool_t m_nsEnabled;
    bool_t m_hcEnabled;
    cloopenwebrtc::AgcModes m_agcMode;
    cloopenwebrtc::EcModes m_ecMode;
    cloopenwebrtc::NsModes m_nsMode;
    bool_t m_dtxEnabled;
    int m_videoBitRates;



    //sean 20130509
    bool tls_enable;
    bool srtp_enable;
    bool user_mode;
    cloopenwebrtc::ccp_srtp_crypto_suite_t encryptType;
    char *m_SrtpKey; //秘钥

    char local_addr[64];

    bool processAudioData;
    bool processOriginalAudioData;

	unsigned int speaker_volume;

	int m_AudioChannelIDDump;
	int m_VideoChannelIDDump;

	int m_SnapshotDeviceId;
	int m_SnapshotChannelID;
    int m_SnapshotDstWidth;
	int m_SnapshotDstHeight;

	bool_t	audioNackEnabled;
	bool_t	videoNackEnabled;
	int		videoProtectionMode;
#ifdef ENABLE_REMB_TMMBR_CONFIG
	bool_t	p2pEnabled;
	bool_t  rembEnabled;
	bool_t  tmmbrEnabled;
#endif
	int m_videoModeChoose;
	int m_desktop_width;
	int m_desktop_height;
	int m_desktop_frame_rate;
	int m_desktop_bit_rate;
  void * m_desktop_activity;
  
	int m_VideoTimeOut;
//#ifdef WIN32
//	ChromaKeyFilter * m_ChromaKeyFilter;
//#endif


public:
//Sean add begin 20131022 for video fast update in video conference
    int serphone_send_key_frame(SerPhoneCall *call);
//Sean add end 20131022 for video fast update in video conference

//Sean add begin 20131119 noise suppression
    int serphone_noise_suppression(const void* audioSamples,
                                   WebRtc_Word16 *out);
//Sean add end 20131119 noise suppression

//Sean add begin 20140322 for windows XP
private:
    bool softMuteStatus;
public:
    int serphone_soft_mute(SerPhoneCall *call, bool mute);
    bool serphone_soft_get_mute_status(SerPhoneCall *call);
//Sean add begin 20140322 for windows XP

//sean add begin 20140422 SetAudioGain
public:
    int serphone_set_audio_gain(float inaudio_gain, float outaudio_gain);
//sean add end 20140422 SetAudioGain
//sean add begin 20140424 SetPrivateCloud
private:
    char privateCloudCheckAddress[100];
    char privateCloudCampanyID[200];
    bool nativeCheck;
public:
    const char *serphone_get_privateCloudCheckAddress();
    const char *serphone_get_privateCloudCompanyID();
    int serphone_set_privateCloud(const char *companyID,const char *restAddr, bool isNativeCheck);
    bool serphone_get_nativeCheck();
//sean add end 20140424 SetPrivateCloud

//sean add begin 20140505 tls root ca
private:
    char *rootCaPath;
public:
    int serphone_set_root_ca_path(const char* caPath);
//sean add end 20140505 tls root ca

//sean add begin 20140508 srtp
private:
    bool userKeySetted;
public:
    void serphone_core_enable_srtp(bool tls, bool srtp, int cryptType, const char *pkey);
//sean add end 20140508 srtp


//sean add begin 20140512 transfer call
private:
    bool isRefering;
    char *referTo;
public:
    void serphone_set_isRefering(bool flag);
    bool serphone_get_isRefering();
    int serphone_set_referTo(const char *referNo);
    char * serphone_get_referTo();
//sean add end 20140512 transfer call
//sean add begin 20140626 init and release audio device
    int serphone_register_audio_device();
    int serphone_deregister_audio_device();
//sean add end 20140626 init and release audio device
//sean add begin 20140626 set group ID for IP route
private:
    char *groupID;
    char *networkType;
    char *groupIDAndNetworkType;
public:
    int serphone_set_groupID(const char *group);
    int serphone_set_networkType(const char *type);
    const char *serphone_get_groupID();
//    const char *serphone_get_networkType();
//sean add end 20140626 set group ID for IP route


//sean add begin 20140616 video conference

private:
    static std::map<int,VideoConferenceDesc*> videoConferenceM;
    std::map<std::string , int> videoConferencePairSipChannel;
    char *videoConferenceIp;
//    int videoConferencePort;
    int CursorVideoConferencePort; //Here means local port cursor
    char *selfSipNo;
    char *remoteSipNo;
    char *registerUserdata;
    char *proxyAddr;
    int proxyPort;
    bool isInVideoConference;
    char conferenceID[100];
    char conferencePsw[100];
public:
    const char * serphone_get_proxyAddr();
    int serphone_get_proxyPort();

private:
    void serphone_check_video_conference_request_failed();
public:
    //sdk设置视频会议的服务器地址
    int serphone_set_video_conference_addr(const char *ip);
    //sdk根据sip号去视频会议服务器请求视??
    int serphone_set_video_window_and_request_video_accord_sip(const char *sipNo, void *videoWindowC, const char *conferenceNo, const char *confPasswd, int port);
	int serphone_stop_conference_video_accord_sip(const char *sipNo, const char *conferenceNo, const char *confPasswd);
	int serphone_set_video_conference_released();
    int serphone_reset_conference_video_window(const char *sipNo, void *newVideoWindowC);
//sean add end 20140616 video conference

//sean add begin 20140705 video conference
public:
	void *createLiveStream();
    int configLiveVideoStream(void *handle, LiveVideoStreamConfig config);
	void setLiveVideoSource(void *handle,int video_source);
	int playLiveStream(void *handle, const char * url, void *renderView);
	int pushLiveStream(void *handle, const char * url, void *renderView);
    // int setLiveStreamNetworkCallBack(void *handle, onLiveStreamNetworkStatusCallBack callback);
	void stopLiveStream(void *handle);
	void releaseLiveStream(void *handle);
	void enableLiveStreamBeauty(void *handle);
	void disableLiveStreamBeauty(void *handle);
	int liveStream_SelectCamera(void *handle, int index);

public:
    virtual void onVideoConference(int channelID, int status, int payload);
//sean add end 20140705 video conference
    const char *serphone_get_self_sipNo();

    virtual void onDtmf(const char *callid, char dtmf);
public:
    int serphone_set_silk_rate(int rate);
public:
    static int serphone_set_traceFlag(/*bool flag*/);//Don't user flag for the time being
    int serphone_set_remote_sip(char *remote);
#if 0
private:
    bool ringbackFlag;
public:
    int serphone_set_ringback(SerPhoneCall *call, bool flag);
#endif

    std::map<std::string, bool> filterDupMessage;


private:
    bool reconnect;
public:
    int serphone_set_reconnect(bool flag);
    bool serphone_get_reconnect();

private:
	//svc encoder parameters
	bool_t m_svcEnabled;
	unsigned int m_spatial_layer_num;
	unsigned int m_temporal_layer_num;
	unsigned int m_origin_video_width;
	unsigned int m_origin_video_height;
	unsigned int m_origin_fps;

//haiyuntong
#ifdef HAIYUNTONG
private:
    bool enableHaiyuntong;
    bool isLandingCall;
    char *confID;
    bool isAudioConf;
    char *deviceID;
    char *appID;
    char *pinCode;
    bool haiyuntongTestFlag;
public:
    int serphone_set_deviceid_pincode(const char *devId, const char *appId, bool testFlag);
    int serphone_enable_haiyuntong(bool flag);
    bool serphone_haiyuntong_enabled();
    int serphone_caller_init_haiyuntong(char *appid, long appidLen,  char *selfSipNo, long selfSipNoLen, char *devId);
    //caller bkey
    int serphone_caller_180_183_transmitKeySet(SalOp *op,char *callee, long calleeLen);
    //callee create bkey
    int serphone_callee_create_bKey(SalOp *op,char *caller, long callerLen);
    int serphone_encrypt(char *mediaStream, long mediaStreamLen, char *selfSipNo, long selfSipNoLen, char *mediaStreamCrp , long* mediaStreamCrpLen);
    int serphone_decrypt(char *mediaStream, long mediaStreamLen, char *remoteSipNo, long remoteSipNoLen, char *mediaStreamDecrp , long* mediaStreamDecrpLen);
    int serphone_delete_transmit_key();
    //audio conf
    int serphone_audio_conf_key_set(SalOp *op);
    int serphone_dump_conf_key(SalOp *op);
    //sms
    int serphone_sms_encrypt(char *sms, long smsLen, char *remoteSip, long remoteSipLen, char *smsCrpt, long* smsCrptLen);
    int serphone_sms_decrypt(char *sms, long smsLen, char *remoteSip, long remoteSipLen, char *smsDeCrpt, long* smsDeCrptLen);
    //group sms
    int serphone_group_sms_encrypt(char *sms, long smsLen, char *proxyAddr, long proxyAddrLen, char *smsCrpt, long* smsCrptLen);
    int serphone_group_sms_decrypt(char *sms, long smsLen, char *proxyAddr, long proxyAddrLen, char *smsDeCrpt, long* smsDeCrptLen);
    //file
    int serphone_file_encrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileCrpEnvelop, long* fileCrpEnvelopLen);
    int serphone_file_decrypt(const unsigned char *file, long fileLen, char *remoteSip, long remoteSipLen, unsigned char *fileDeCrpt, long* fileDeCrptLen);
    //group file
    int serphone_group_file_encrypt(const unsigned char *file, long fileLen, char **userList, long *earchLen, int numOfUsers, unsigned char *groupFileCrpEnvelopp, long* groupFileCrpEnvelopLen);
    int serphone_group_file_decrypt(const unsigned char *file, long fileLen, char *proxyAddr, long proxyAddrLen, unsigned char *groupFileDecrpt, long*groupFileDecrptLen);
    //manange contact
    int serphone_add_contact(char **desid, long *desidLen, int num);
    int serphone_del_contact(char *remoteSip, long remoteSipLen);
    bool serphone_get_isAudioConf();
    int serphone_certExisted(const char *sip, long sipLen);
#endif
private:
		//cloopenwebrtc::SendStatisticsProxy *pSendStats_;
		//cloopenwebrtc::ReceiveStatisticsProxy *pReceiveStats_;
public:
	//add by ylr
	virtual void onReceiverStats(const char *callid, const int framerate, const int bitrate);
	virtual void onIncomingCodecChanged(const char *callid, const int width, const int height);
	int GetBandwidthUsage(const char* callid,
							unsigned int& total_bitrate_sent,
							unsigned int& video_bitrate_sent,
							unsigned int& fec_bitrate_sent,
							unsigned int& nackBitrateSent);

	int GetEstimatedSendBandwidth(const char* callid,
		                    unsigned int* estimated_bandwidth);

	int GetEstimatedReceiveBandwidth(const char* callid,
		                    unsigned int* estimated_bandwidth);

	int GetReceiveChannelRtcpStatistics(const char* callid,
							_RtcpStatistics& basic_stats,
							__int64& rtt_ms);
	int GetSendChannelRtcpStatistics(const char* callid,
		_RtcpStatistics& basic_stats,
		__int64& rtt_ms);

	int GetRtpStatistics(const char* callid,
		_StreamDataCounters& sent,
		_StreamDataCounters& received);

	int GetStatsData(int type, char* callid, void** pbDataArray, int *pArraySize);
	void DeleteStatsData(void *pb_data);
	//int GetSendStats(const char* callid, cloopenwebrtc::VideoSendStream::Stats &sendStats);
    int Serphone_enable_opus_FEC(bool enable);
    int Serphone_set_opus_packet_loss_rate(int rate);
    
    // record local mp4 video.
    int startRecordLocalMedia(const char *fileName, void *localview);
    void stopRecordLocalMedia();

	void setLocalSSRC(unsigned int ssrc);
	int send_tmmbr_request_video(SerPhoneCall *call, uint32_t ssrc);
	void cancel_tmmbr_request_video(SerPhoneCall *call);
	void video_start_receive(SerPhoneCall *call);
	void video_stop_receive(SerPhoneCall *call);
    int set_rotate_captured_frames(int deviceid, ECMediaRotateCapturedFrame tr);
    //---begin
	private:
	//cloopenwebrtc::VideoSendStream::Config CreateVideoSendStreamConfig();
	//cloopenwebrtc::SendStatisticsProxy* Serphone_set_video_send_statistics_proxy(int video_channel);
	//cloopenwebrtc::ReceiveStatisticsProxy* Serphone_set_video_receive_statistics_porxy(int video_channel);
	//---end
private:
    bool m_enable_fec;
    int m_opus_packet_loss_rate;
	unsigned int m_localSSRC;
};

SerphoneAddress * serphone_address_new(const char *uri);
SerphoneAddress * serphone_address_clone(const SerphoneAddress *uri);
const char *serphone_address_get_scheme(const SerphoneAddress *u);
const char *serphone_address_get_display_name(const SerphoneAddress* u);
const char *serphone_address_get_username(const SerphoneAddress *u);
const char *serphone_address_get_domain(const SerphoneAddress *u);
/**
 * Get port number as an integer value.
 *
 */
int serphone_address_get_port_int(const SerphoneAddress *u);
/**
 * Get port number, null if not present.
 */
const char* serphone_address_get_port(const SerphoneAddress *u);
void serphone_address_set_display_name(SerphoneAddress *u, const char *display_name);
void serphone_address_set_username(SerphoneAddress *uri, const char *username);
void serphone_address_set_domain(SerphoneAddress *uri, const char *host);
void serphone_address_set_port(SerphoneAddress *uri, const char *port);
void serphone_address_set_port_int(SerphoneAddress *uri, int port);
/*remove tags, params etc... so that it is displayable to the user*/
void serphone_address_clean(SerphoneAddress *uri);
char *serphone_address_as_string(const SerphoneAddress *u);
char *serphone_address_as_string_uri_only(const SerphoneAddress *u);
bool_t serphone_address_weak_equal(const SerphoneAddress *a1, const SerphoneAddress *a2);
void serphone_address_destroy(SerphoneAddress *u);

/////////////////////////////param///////////////////////
void serphone_call_params_destroy(SerphoneCallParams *p);
SerphoneCallParams * serphone_call_params_copy(const SerphoneCallParams *cp);

/////////////////////////////stream////////////////////////////
bool_t media_parameters_changed(SerPhoneCall *call, SalMediaDescription *oldmd, SalMediaDescription *newmd);

//////////////////authen
void serphone_auth_info_destroy(SerphoneAuthInfo **obj);
SerphoneAuthInfo *serphone_auth_info_new(const char *username, const char *userid,
				   										const char *passwd, const char *ha1,const char *realm);
SerphoneAuthInfo *serphone_auth_info_clone(const SerphoneAuthInfo *ai);
const char *serphone_auth_info_get_username(const SerphoneAuthInfo *i);
const char *serphone_auth_info_get_passwd(const SerphoneAuthInfo *i);
const char *serphone_auth_info_get_userid(const SerphoneAuthInfo *i);
void serphone_auth_info_set_passwd(SerphoneAuthInfo *info, const char *passwd);
void serphone_auth_info_set_username(SerphoneAuthInfo *info, const char *username);
void serphone_auth_info_set_userid(SerphoneAuthInfo *info, const char *userid);
SerphoneAuthInfo *serphone_auth_info_new_from_config_file(LpConfig * config, int pos);
void serphone_auth_info_write_config(LpConfig *config, SerphoneAuthInfo *obj, int pos);
void serphone_proxy_config_write_to_config_file(LpConfig *config, SerphoneProxyConfig *obj, int index);
bool_t transports_unchanged(const LCSipTransports * tr1, const LCSipTransports * tr2);

void serphone_proxy_config_update(SerphoneProxyConfig **cfg);
// add by yuanfang
void *serphone_core_get_user_data(ServiceCore *lc);
void serphone_core_set_user_data(ServiceCore *lc, void *userdata);
void serphone_core_set_android_objects(void* javaVM, void* env, void* context);

int serphone_core_get_unique_id(char *uniqueid, int len);

void serphone_core_set_bind_local_addr(const char* addr);
#endif
