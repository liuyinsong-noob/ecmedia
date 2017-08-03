//
//  ECMedia.h
//  servicecoreVideo
//
//  Created by Sean Lee on 15/6/8.
//
//
#ifndef __servicecoreVideo__ECMedia__
#define __servicecoreVideo__ECMedia__
#include <stdio.h>
#include "sdk_common.h"
#include "common_types.h"
#ifdef WEBRTC_ANDROID
#include "jni.h"
#define ECMEDIA_API JNIEXPORT
#elif  defined(WIN32)
#define ECMEDIA_API  _declspec(dllexport)
#else
#define ECMEDIA_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*ReturnVideoWidthHeightM)(int width,int height, int channelid);
typedef int (*onEcMediaReceivingDtmf)(int channelid, char dtmfch);//dtmf
typedef int (*onEcMediaPacketTimeout)(int channelid);
typedef int (*onEcMediaStunPacket)(int channelid, void *data, int len, const char *fromIP, int fromPort, bool isRTCP, bool isVideo);
typedef int (*onEcMediaAudioData)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
typedef int (*onEcMediaVideoDataV)(int channelid, const void *data, int inLen, void *outData, int &outLen, bool send);
typedef int (*onEcMediaVideoConference)(int channelid, int status, int payload);
typedef int (*onEcMediaRequestKeyFrameCallback)(const int channelid);
typedef int (*onVoeCallbackOnError)(int channelid, int errCode);
typedef int(*onSoundCardOn)(int deviceType);//0, playout; 1, record
typedef int(*onEcMediaDesktopCaptureErrCode)(int desktop_capture_id, int errCode);
typedef int (*onEcMediaShareWindowSizeChange)(int desktop_capture_id, int width, int height);
typedef int(*onEcMediaNoCameraCaptureCb)(const int id, const bool capture);
enum NET_STATUS_CODE {
    RTMP_STATUS_CONNECTING = 1,
    RTMP_STATUS_CONNECTED_SUCCESS,
    RTMP_STATUS_CONNECTED_FAILED,
    RTMP_STATUS_TIMEOUT,
    RTMP_STSTUS_PUSH_SUCCESS,
    RTMP_STSTUS_PUSH_FAILED,
    RTMP_STSTUS_PLAY_SUCCESS,
    RTMP_STSTUS_PLAY_FAILED
};
typedef int(*onLiveStreamNetworkStatusCallBack)(void *handle, NET_STATUS_CODE code);
typedef int(*onLiveStreamVideoResolution)(void *handle, int width, int height);
/*
 * Enable trace.
 */
ECMEDIA_API int ECMedia_set_trace(const char *logFileName,void *printhoolk,int level, int lenMb);
ECMEDIA_API int ECMedia_un_trace();
ECMEDIA_API const char* ECMedia_get_Version();
ECMEDIA_API void PrintConsole(const char * fmt,...);
/*
 *1
 */
ECMEDIA_API void ECMedia_set_android_objects(void* javaVM, void* env, void* context);
ECMEDIA_API int ECMedia_ring_start(int& channelid, const char *filename, bool loop);
/*
 *
 */
ECMEDIA_API int ECMedia_ring_stop(int& channelid);
/*
 *
 @return: 0: success
 1: already init
 -99:
 VE_AUDIO_DEVICE_MODULE_ERROR: audio device init failed.
 */
ECMEDIA_API int ECMedia_init_audio();
/*
 *1
 */
ECMEDIA_API int ECMedia_uninit_audio();
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_create_channel(int& channelid, bool is_video);
/*
 *1
 */
ECMEDIA_API bool ECMedia_get_recording_status();
/*
 *1
 */
ECMEDIA_API int ECMedia_delete_channel(int& channelid, bool is_video);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_local_receiver(int channelid, int rtp_port, int rtcp_port = -1);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_set_send_destination(int channelid, int rtp_port, const char *rtp_addr, int source_port, int rtcp_port, const char *rtcp_ipaddr);
/**
 *
 */
ECMEDIA_API int ECMedia_audio_set_socks5_send_data(int channel_id, unsigned char *data, int length, bool isRTCP);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_start_receive(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_stop_receive(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_start_send(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_stop_send(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_Register_voice_engine_observer(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_DeRegister_voice_engine_observer();
/*
 *
 */
ECMEDIA_API int ECMedia_set_AgcStatus(bool agc_enabled, cloopenwebrtc::AgcModes agc_mode);
/*
 *
 */
ECMEDIA_API int ECMedia_set_EcStatus(bool ec_enabled, cloopenwebrtc::EcModes ec_mode);
/*
 *
 */
ECMEDIA_API int ECMedia_set_NsStatus(bool ns_enabled, cloopenwebrtc::NsModes ns_mode);
/*
 *
 */
ECMEDIA_API int ECMedia_set_SetAecmMode(cloopenwebrtc::AecmModes aecm_mode, bool cng_enabled);
ECMEDIA_API int ECMedia_EnableHowlingControl(bool enabled);
ECMEDIA_API int ECMedia_IsHowlingControlEnabled(bool &enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_set_packet_timeout_noti(int channel, int timeout);
/*
 *
 */
ECMEDIA_API int ECMedia_get_packet_timeout_noti(int channel, bool& enabled, int& timeout);
/*
 *
 */
ECMEDIA_API int ECMedia_set_network_type(int audio_channelid, int video_channelid, const char *type);
/*
 *
 */
ECMEDIA_API int ECMedia_get_network_statistic(int channelid_audio, int channelid_video, long long *duration, long long *sendTotalSim, long long *recvTotalSim, long long *sendTotalWifi, long long *recvTotalWifi);
/*
 *
 */
ECMEDIA_API int ECMedia_set_MTU(int channelid, int mtu);
/*
 *
 */
ECMEDIA_API int ECMedia_set_video_rtp_keepalive(int channelid, bool enable, int interval, int payloadType);

ECMEDIA_API int ECMedia_video_set_local_ssrc(int channelid, unsigned int ssrc);

ECMEDIA_API int ECMedia_video_request_remote_ssrc(int channelid, unsigned int ssrc);

ECMEDIA_API int ECMedia_video_cancel_remote_ssrc(int channelid);

ECMEDIA_API int ECMedia_set_audio_rtp_keepalive(int channelid, bool enable, int interval, int payloadType);
/*
 *
 */
ECMEDIA_API int ECMedia_set_NACK_status(int channelid, bool enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_set_RTCP_status(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_get_media_statistics(int channelid, bool is_video, MediaStatisticsInfo& call_stats);
/*
 *
 */
ECMEDIA_API int ECMedia_start_rtp_dump(int channelid, bool is_video, const char *file, cloopenwebrtc::RTPDirections dir);
/*
 *
 */
ECMEDIA_API int ECMedia_stop_rtp_dump(int channelid, bool is_video, cloopenwebrtc::RTPDirections dir);
/*
 *
 */
ECMEDIA_API int ECMedia_get_playout_device_num(int& speaker_count);
/*
 *
 */
ECMEDIA_API int ECMedia_get_specified_playout_device_info(int index, char *name, char *guid);
/*
 *
 */
ECMEDIA_API int ECMedia_select_playout_device(int index);
/*
 *
 */
ECMEDIA_API int ECMedia_get_record_device_num(int& microphone_count);
/*
 *
 */
ECMEDIA_API int ECMedia_get_specified_record_device_info(int index, char *name, char *guid);
/*
 *
 */
ECMEDIA_API int ECMedia_select_record_device(int index);
/*
 *
 */
ECMEDIA_API int ECMedia_set_loudspeaker_status(bool enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_get_loudpeaker_status(bool& enabled);
/*
 *
 */
ECMEDIA_API int ECMedia_reset_audio_device();
///*
// *
// */
//ECMEDIA_API int ECMedia_init_srtp(int channelid);
///*
// *Âä†ÂØÜ
// */
//ECMEDIA_API int ECMedia_enable_srtp_receive(int channelid, const char *key);
///*
// *
// */
//ECMEDIA_API int ECMedia_enable_srtp_send(int channelid, const char *key);
///*
// *
// */
//ECMEDIA_API int ECMedia_shutdown_srtp(int channel);
/*
 *
 */
ECMEDIA_API int ECMedia_set_speaker_volume(int volumep);
/*
 *
 */
ECMEDIA_API int ECMedia_get_speaker_volume(unsigned int& volumep);
/*
 *
 */
ECMEDIA_API int ECMedia_set_mic_volume(int volumep);
/*
 *
 */
ECMEDIA_API int ECMedia_get_mic_volume(unsigned int& volumep);
/*
 *
 */
ECMEDIA_API int ECMedia_set_mute_status(bool mute);
/*
 *
 */
ECMEDIA_API int ECMedia_get_mute_status(bool& mute);
/*
 *
 */
ECMEDIA_API int ECMedia_set_speaker_mute_status(bool mute);
/*
 *
 */
ECMEDIA_API int ECMedia_get_speaker_mute_status(bool& mute);
/*
 *
 */
ECMEDIA_API int ECMedia_num_of_supported_codecs_audio();
/*
 *
 */
ECMEDIA_API int ECMedia_get_supported_codecs_audio(cloopenwebrtc::CodecInst codecs[]);
/*
 *1
 */
ECMEDIA_API int ECMedia_get_send_codec_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_send_codec_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_receive_playloadType_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_get_receive_playloadType_audio(int channelid, cloopenwebrtc::CodecInst& audioCodec);
/*
 *1
 */
ECMEDIA_API int ECMedia_set_VAD_status(int channelid, cloopenwebrtc::VadModes mode, bool dtx_enabled);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_start_playout(int channelid);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_stop_playout(int channelid);
ECMEDIA_API int ECMedia_audio_start_record();
ECMEDIA_API int ECMedia_audio_stop_record();
ECMEDIA_API int ECMedia_set_soundcard_on_cb(onSoundCardOn soundcard_on_cb);
/*
 *Â≠óÁ¨¶ ËØ≠Èü≥ÈÄöÈÅì
 */
ECMEDIA_API int ECMedia_send_dtmf(int channelid, const char dtmfch);
ECMEDIA_API int ECMedia_set_send_telephone_event_payload_type(int channelid, unsigned char type);
ECMEDIA_API int ECMedia_set_recv_telephone_event_payload_type(int channelid, unsigned char type);
/*
 *ÊØèÊ¨°‰∫ßÁîüchannelÂêé‰º†ËøõÊù•
 */
ECMEDIA_API int ECMedia_set_dtmf_cb(int channelid, onEcMediaReceivingDtmf dtmf_cb);
ECMEDIA_API int ECMedia_set_media_packet_timeout_cb(int channelid, onEcMediaPacketTimeout media_timeout_cb);
ECMEDIA_API int ECMedia_set_stun_cb(int channelid, onEcMediaStunPacket stun_cb);
ECMEDIA_API int ECMedia_set_audio_data_cb(int channelid, onEcMediaAudioData audio_data_cb);
/**
 * ËÆæÁΩÆÈü≥È¢ë PCM Êï∞ÊçÆÂõûË∞É
 * @param channelid: channel id
 * @param callback : callback ÂÖ∑‰ΩìËß£ÈáäËßÅÂÆö‰πâÂ§Ñ
 * @return ÊàêÂäüËøîÂõû0Ôºå Â§±Ë¥•ËøîÂõûÈùû0
 */
ECMEDIA_API int ECMedia_set_pcm_audio_data_cb(int channelid, cloopenwebrtc::ECMedia_PCMDataCallBack callback);
ECMEDIA_API int ECMedia_set_video_data_cb(int channelid, onEcMediaVideoDataV Video_data_cb);
ECMEDIA_API int ECMedia_set_voe_cb(int channelid, onVoeCallbackOnError voe_callback_cb);
/*
 * ONLY USE FOR PEER CONNECTION FOR AUDIO
 */
ECMEDIA_API int ECMedia_sendRaw(int channelid, int8_t *data, uint32_t length, bool isRTCP, uint16_t port = 0, const char* ip = NULL);
ECMEDIA_API int ECMedia_EnableIPV6(int channel, bool flag);
ECMEDIA_API int ECMedia_IsIPv6Enabled(int channel);
ECMEDIA_API int ECMedia_AmrNBCreateEnc();
ECMEDIA_API int ECMedia_AmrNBCreateDec();
ECMEDIA_API int ECMedia_AmrNBFreeEnc();
ECMEDIA_API int ECMedia_AmrNBFreeDec();
ECMEDIA_API int ECMedia_AmrNBEncode(short* input, short len, short*output, short mode);
ECMEDIA_API int ECMedia_AmrNBEncoderInit(short dtxMode);
ECMEDIA_API int ECMedia_AmrNBDecode(short* encoded, int len, short* decoded);
ECMEDIA_API int ECMedia_AmrNBVersion(char *versionStr, short len);
//SRTP
ECMEDIA_API int ECMedia_init_srtp_audio(int channel);
ECMEDIA_API int ECMedia_shutdown_srtp_audio(int channel);
ECMEDIA_API int ECMedia_enable_srtp_send_audio(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type,	const char* key);
ECMEDIA_API int ECMedia_disable_srtp_send_audio(int channel);
ECMEDIA_API int ECMedia_enable_srtp_recv_audio(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type,	const char* key);
ECMEDIA_API int ECMedia_disable_srtp_recv_audio(int channel);
ECMEDIA_API int ECMedia_start_record_playout(int channel, char *filename);
ECMEDIA_API int ECMedia_stop_record_playout(int channel);
ECMEDIA_API int ECMedia_start_record_microphone(char *filename);
ECMEDIA_API int ECMedia_stop_record_microphone();
ECMEDIA_API int ECMedia_start_record_send_voice(char *filename);
ECMEDIA_API int ECMedia_stop_record_send_voice();
#ifdef VIDEO_ENABLED
/*
 *
 */
ECMEDIA_API int ECMedia_init_video();
/*
 *
 */
ECMEDIA_API int ECMedia_uninit_video();
/*
 *
 */
ECMEDIA_API int ECMdeia_num_of_capture_devices();
ECMEDIA_API int ECMedia_get_capture_device(int index, char *name, int name_len, char *id, int id_len);
/*
 *
 */
ECMEDIA_API int ECMedia_num_of_capabilities(const char *id, int id_len);
/*
 *
 */
ECMEDIA_API int ECMedia_get_capture_capability(const char *id, int id_len, int index, CameraCapability& capabilityp);
/*
 *
 */
ECMEDIA_API int ECMedia_allocate_capture_device(const char *id, size_t len, int& deviceid);
/*
 *
 */
ECMEDIA_API int ECMedia_connect_capture_device(int deviceid, int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_getOrientation(const char *id, ECMediaRotateCapturedFrame &tr);
/*
 *
 */
ECMEDIA_API int ECMedia_set_rotate_captured_frames(int deviceid, ECMediaRotateCapturedFrame tr);
/*
 *
 */
ECMEDIA_API int ECMedia_start_capture(int deviceid, CameraCapability cam);
ECMEDIA_API int ECMedia_video_set_local_receiver(int channelid, int rtp_port, int rtcp_port = -1, bool ipv6 = false);
/*
 *
 */
ECMEDIA_API int ECMedia_video_set_send_destination(int channelid, const char *rtp_addr, int rtp_port, const char *rtcp_addr, int rtcp_port);
/**
 *
 */
ECMEDIA_API int ECMedia_video_set_socks5_send_data(int channel_id, unsigned char *data, int length, bool isRTCP);
/*
 *
 */
ECMEDIA_API int ECMedia_video_start_receive(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_video_stop_receive(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_video_start_send(int channelid);
/*
 *
 */
ECMEDIA_API int ECMedia_video_stop_send(int channelid);

/**
 * ËÆæÁΩÆËßÜÈ¢ëÂéüÂßãÊï∞ÊçÆÂõûË∞É
 * @param channelid: channel id.
 * @param callback : ÂéüÂßãËßÜÈ¢ëÊï∞ÊçÆÂõûË∞ÉÂáΩÊï∞ÊåáÈíà
 * @return ÊàêÂäüËøîÂõû0ÔºåÂ§±Ë¥•ËøîÂõûÈùû0
 */
ECMEDIA_API int ECMedia_set_i420_framecallback(int channelid, cloopenwebrtc::ECMedia_I420FrameCallBack callback);

/*
 *
 */
ECMEDIA_API int ECMedia_set_local_video_window(int deviceid, void *video_window);
/*
 *
 */
ECMEDIA_API int ECMedia_stop_capture(int captureid);
/*
 */
ECMEDIA_API int ECMedia_allocate_desktopShare_capture(int& desktop_captureid, int capture_type);

/*
 */
/*
 */
ECMEDIA_API int ECMedia_get_screen_list(int desktop_captureid, ScreenID **screenList);
ECMEDIA_API int ECMedia_get_window_list(int desktop_captureid, WindowShare **windowList);
/*
 //Ê°åÈù¢ÂÖ±‰∫´Ë∞ÉÁî®‰æãÂ≠ê
 ECMedia_allocate_desktopShare_capture(call->m_desktopShareDeviceId, type);
 getShareScreenInfo(&screenId, call->m_desktopShareDeviceId);
 getShareWindowInfo(&windowInfo, call->m_desktopShareDeviceId);
 ECMedia_select_screen(call->m_desktopShareDeviceId, m_pScreenInfo[0]);
 ECMedia_connect_desktop_captureDevice(call->m_desktopShareDeviceId, call->m_VideoChannelID);
 ECMedia_start_desktop_capture(call->m_desktopShareDeviceId, 15);
 ...
 ECMedia_disconnect_desktop_captureDevice(call->m_VideoChannelID);
 ECMedia_stop_desktop_capture(call->m_desktopShareDeviceId);
 ECMedia_release_desktop_capture(call->m_desktopShareDeviceId);
 */
ECMEDIA_API bool ECMedia_select_screen(int desktop_captureid, ScreenID screeninfo);
ECMEDIA_API bool ECMedia_select_window(int desktop_captureid, WindowID WindowInfo);
ECMEDIA_API int ECMedia_start_desktop_capture(int desktop_captureid, int fps);
/*
 * Should not invoke from main UI thread.
 */
ECMEDIA_API int ECMedia_stop_desktop_capture(int desktop_captureid);
ECMEDIA_API int ECMedia_release_desktop_capture(int desktop_captureid);
ECMEDIA_API int ECMedia_connect_desktop_captureDevice(int desktop_captureid, int video_channelId);
ECMEDIA_API int ECMedia_disconnect_desktop_captureDevice(int video_channelId);
ECMEDIA_API int ECMedia_set_desktop_share_err_code_cb(int desktop_captureid, int channelid, onEcMediaDesktopCaptureErrCode capture_err_code_cb);
ECMEDIA_API int ECMedia_set_desktop_share_window_change_cb(int desktop_captureid, int channelid, onEcMediaShareWindowSizeChange share_window_change_cb);
ECMEDIA_API int ECMedia_get_desktop_capture_size(int desktop_captureid, int &width, int &height);
ECMEDIA_API int ECMedia_set_screen_share_activity(int desktop_captureid, void* activity);
/*
 *
 */
ECMEDIA_API int ECMedia_add_render(int channelid, void *video_window, ReturnVideoWidthHeightM videoResolutionCallback);
/*
 *
 */
ECMEDIA_API int ECMedia_stop_render(int channelid, int deviceid);
/*
 *
 */
ECMEDIA_API int ECMedia_num_of_supported_codecs_video();
/*
 *
 */
ECMEDIA_API int ECMedia_get_supported_codecs_video(cloopenwebrtc::VideoCodec codecs[]);
/*
 *
 */
ECMEDIA_API int ECMedia_set_key_frame_request_cb(int channelid, bool isVideoConf,onEcMediaRequestKeyFrameCallback cb);
/*
 *
 */
ECMEDIA_API int ECMedia_set_send_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
/*
 *
 */
ECMEDIA_API int ECMedia_get_send_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
/*
 *
 */
ECMEDIA_API int ECMedia_set_receive_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
#ifdef ENABLE_FEC_TEST
ECMEDIA_API int ECMedia_set_receive_codec_video_fec(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
#endif
/*
 *
 */
ECMEDIA_API int ECMedia_get_receive_codec_video(int channelid, cloopenwebrtc::VideoCodec& videoCodec);
/**
 * description: ËÆæÁΩÆÂ∫ïÂ±ÇyuvËßÜÈ¢ëÂ∏ßÁöÑÁº©ÊîæÊñπÂºè
 * FrameScaleType:
 *   kScaleTypeCropping -> ‰ª•Ë£ÅÂâ™ÊñπÂºèÁº©ÊîæËßÜÈ¢ëÂ∏ß
 *   kScaleTypeFilling  -> ‰ª•Â°´ÂÖÖÊñπÂºèÁº©ÊîæËßÜÈ¢ëÂ∏ß
 */
ECMEDIA_API int ECMedia_set_frame_scale_type(int channelid, cloopenwebrtc::FrameScaleType type);
    
ECMEDIA_API int ECMedia_set_video_qm_mode(int channelid, cloopenwebrtc::VCMQmResolutionMode mode);
    
ECMEDIA_API int ECMedia_set_video_conf_cb(int channelid, onEcMediaVideoConference video_conf_cb);
ECMEDIA_API int ECMedia_set_stun_cb_video(int channelid, onEcMediaStunPacket stun_cb);
/*
 * ONLY USE FOR PEER CONNECTION FOR VIDEO
 */
ECMEDIA_API int ECMedia_sendUDPPacket(const int channelid,const void* data,const unsigned int length,int& transmitted_bytes,bool use_rtcp_socket = false,uint16_t port = 0,const char* ip = NULL);
ECMEDIA_API int ECMedia_set_NACK_status_video(int channelid, bool enabled);
ECMEDIA_API int ECMedia_set_RTCP_status_video(int channelid,int mode);
ECMEDIA_API int ECMedia_setVideoConferenceFlag(int channel,const char *selfSipNo ,const char *sipNo, const char *conferenceNo, const char *confPasswd, int port, const char *ip);
ECMEDIA_API int ECMedia_send_key_frame(int channel);
ECMEDIA_API int ECMedia_video_EnableIPV6(int channel, bool flag);
ECMEDIA_API int ECMedia_video_IsIPv6Enabled(int channel);
ECMEDIA_API int ECMedia_set_FEC_status_video(const int channelid,
                                             const bool enable,
                                             const unsigned char payload_typeRED,
                                             const unsigned char payload_typeFEC);
ECMEDIA_API int ECMedia_set_HybridNACKFEC_status_video(const int channelid,
                                                       const bool enable,
                                                       const unsigned char payload_typeRED,
                                                       const unsigned char payload_typeFEC);
ECMEDIA_API int ECMedia_start_record_screen(int audioChannel, const char* filename, int bitrates, int fps, int screen_index);
ECMEDIA_API int ECMedia_start_record_screen_ex(int audioChannel, const char* filename, int bitrates, int fps, int screen_index, int left, int top, int width, int height);
ECMEDIA_API int ECMedia_stop_record_screen(int audioChannel);
ECMEDIA_API int ECMedia_start_record_remote_video(int audioChannel, int videoChannel, const char* filename);
ECMEDIA_API int ECMedia_stop_record_remote_video(int audioChannel, int videoChannel);
ECMEDIA_API int ECMedia_start_record_local_video(int audioChannel, int videoChannel, const char* filename);
ECMEDIA_API int ECMedia_stop_record_local_video(int audioChannel, int videoChannel);
//capture device id
ECMEDIA_API int ECMedia_get_local_video_snapshot(int deviceid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
ECMEDIA_API int ECMedia_save_local_video_snapshot(int deviceid, const char* filePath);
//video channel id
ECMEDIA_API int ECMedia_get_remote_video_snapshot(int channelid, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height);
ECMEDIA_API int ECMedia_save_remote_video_snapshot(int channelid, const char* filePath);
ECMEDIA_API int ECmedia_enable_deflickering(int captureid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableColorEnhancement(int channelid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableDenoising(int captureid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableBrightnessAlarm(int captureid, bool enable);
ECMEDIA_API int ECmedia_enable_EnableBeautyFilter(int captureid, bool enable);
//SRTP
ECMEDIA_API int ECMedia_init_srtp_video(int channel);
ECMEDIA_API int ECMedia_shutdown_srtp_video(int channel);
ECMEDIA_API int ECMedia_enable_srtp_send_video(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key);
ECMEDIA_API int ECMedia_disable_srtp_send_video(int channel);
ECMEDIA_API int ECMedia_enable_srtp_recv_video(int channel, cloopenwebrtc::ccp_srtp_crypto_suite_t crypt_type, const char* key);
ECMEDIA_API int ECMedia_disable_srtp_recv_video(int channel);
#endif
ECMEDIA_API int ECMedia_set_CaptureDeviceID(int videoCapDevId);
ECMEDIA_API int ECMedia_Check_Record_Permission(bool &enabled);
ECMEDIA_API int ECmedia_set_shield_mosaic(int video_channel, bool flag);
/* LiveSteam
 ËßÇÁúãÁõ¥Êí≠Ë∞ÉÁî®ËøáÁ®ãÔºö
 void *handle = ECMedia_createLiveStream(0);
 ECMedia_setLiveStreamNetworkCallBack(statusCallback);
 ECMedia_playLiveStream(handle, "http://livestream.com", wndPtr, callback);
 ..
 ECMedia_stopLiveStream(handle);
 ECMedia_releaseLiveStream(handle);
 handle = NULL;
 Áõ¥Êí≠Êé®ÊµÅË∞ÉÁî®ËøáÁ®ãÔºö
 void *handle = ECMedia_createLiveStream(0);
 ECMedia_setVideoProfileLiveStream(handle, cameraIndex, capability, bitrates);
 ECMedia_setLiveStreamNetworkCallBack(statusCallback);
 ECMedia_pushLiveStream(handle, "http://livestream.com", wndPtr);
 ..
 ECMedia_stopLiveStream(handl);
 ECMedia_releaseLiveStream(handle);
 handle = NULL;
 */
/*
 ÂäüËÉΩ     : ÂàõÂª∫Áõ¥Êí≠Ê®°Âùó
 ÂèÇÊï∞     : [IN]  type	  : Á±ªÂûãÔºåÂøÖÈ°ª‰∏∫0
 ËøîÂõûÂÄº   : ËøîÂõûÂÄºÁõ¥Êí≠Ê®°ÂùóÂè•ÊüÑ
 */
ECMEDIA_API void*ECMedia_createLiveStream(int type);
/*
 ÂäüËÉΩ     : ÂºÄÂßãËßÇÁúãÁõ¥Êí≠
 ÂèÇÊï∞     : [IN]  handle		Ôºö Âè•ÊüÑ
 [IN]  url			 : Áõ¥Êí≠Âú∞ÂùÄ
 [IN]  renderView	ÔºöËßÜÈ¢ëÁ™óÂè£
 [IN]  callback		ÔºöËßÜÈ¢ëÂÆΩÈ´òÂõûË∞É
 ËøîÂõûÂÄº   : ËøîÂõûÂÄº 0ÔºöÊàêÂäü  -1ÔºöÂàùÂßãÂåñËµÑÊ∫êÂ§±Ë¥• -2ÔºöÂ∑≤ÁªèÂú®Áõ¥Êí≠ÊàñÊé®ÊµÅ  -3ÔºöËøûÊé•Â§±Ë¥•  -4ÔºöÂª∫Á´ãÊµÅÂ§±Ë¥•
 */
ECMEDIA_API int  ECMedia_playLiveStream(void *handle, const char * url, void *renderView, onLiveStreamVideoResolution callback);
/*
 ÂäüËÉΩ     : ÂºÄÂßãÁõ¥Êí≠Êé®ÊµÅ
 ÂèÇÊï∞     : [IN]  handle		Ôºö Âè•ÊüÑ
 [IN]  url			 : Êé®ÊµÅÂú∞ÂùÄ
 [IN]  renderView	ÔºöÊú¨Âú∞ËßÜÈ¢ëÁ™óÂè£
 ËøîÂõûÂÄº   : ËøîÂõûÂÄº 0ÔºöÊàêÂäü„ÄÄ-1ÔºöÂàùÂßãÂåñËµÑÊ∫êÂ§±Ë¥• -2ÔºöÂ∑≤ÁªèÂú®Áõ¥Êí≠ÊàñËÄÖÊé®ÊµÅ  -3ÔºöËøûÊé•Â§±Ë¥•  -4ÔºöÂª∫Á´ãÊµÅÂ§±Ë¥•
 */
ECMEDIA_API int  ECMedia_pushLiveStream(void *handle, const char * url, void *renderView);
/*
 ÂäüËÉΩ     : ÂÅúÊ≠¢ËßÇÁúãÊàñÊé®ÊµÅ
 ÂèÇÊï∞     :	  [IN]  handle		Ôºö Âè•ÊüÑ
 */
ECMEDIA_API void ECMedia_stopLiveStream(void *handle);
/*
 ÂäüËÉΩ     : ÈáäÊîæÁõ¥Êí≠Ê®°Âùó
 ÂèÇÊï∞     :	  [IN]  handle		Ôºö Âè•ÊüÑ
 */
ECMEDIA_API void ECMedia_releaseLiveStream(void *handle);
/*
 ÂäüËÉΩ     : ÂºÄÂßãÁõ¥Êí≠Ê®°ÂùóÁæéÈ¢ú
 ÂèÇÊï∞     :	  [IN]  handle		Ôºö Âè•ÊüÑ
 */
ECMEDIA_API void ECMedia_enableLiveStreamBeauty(void *handle);
/*
 ÂäüËÉΩ     : ÂÅúÊ≠¢Áõ¥Êí≠Ê®°ÂùóÁæéÈ¢ú
 ÂèÇÊï∞     :	  [IN]  handle		Ôºö Âè•ÊüÑ
 */
ECMEDIA_API void ECMedia_disableLiveStreamBeauty(void *handle);
/*
 ÂäüËÉΩ     : ËÆæÁΩÆÊé®ÊµÅËßÜÈ¢ëÂèÇÊï∞
 ÂèÇÊï∞     : [IN]  handle		Ôºö Âè•ÊüÑ
 [IN]  cameraIndex			 : ÊëÑÂÉèÂ§¥index
 [IN]  cam			 : ËßÜÈ¢ëËÉΩÂäõ
 [IN]  bitrates	ÔºöËßÜÈ¢ëÁ†ÅÁéá
 ËøîÂõûÂÄº   : ËøîÂõûÂÄº 0ÔºöÊàêÂäü„ÄÄ-1ÔºöÂèÇÊï∞‰∏çÊ≠£Á°Æ
 */
ECMEDIA_API int  ECMedia_setVideoProfileLiveStream(void *handle,int cameraIndex, CameraCapability cam, int bitreates);
/*
 ÂäüËÉΩ     : ËÆæÁΩÆÁõ¥Êí≠ÁΩëÁªúÁä∂ÊÄÅÂõûË∞É
 ÂèÇÊï∞     : [IN]  handle		Ôºö Âè•ÊüÑ
 [IN]  callback	 : ÂõûË∞É
 */
ECMEDIA_API void ECMedia_setLiveStreamNetworkCallBack(void *handle, onLiveStreamNetworkStatusCallBack callback);
/*
 ÂäüËÉΩ     : ËÆæÁΩÆÁõ¥Êí≠Êé®ÊµÅÁöÑËßÜÈ¢ëÊù•Ê∫ê
 ÂèÇÊï∞     : [IN]   handle	Ôºö Âè•ÊüÑ
       [OUT]  windows	Ôºöwindows Êï∞ÁªÑÊåáÈíà
       ËøîÂõûÂÄºÔºö Á™óÂè£‰∏™Êï∞
 */
ECMEDIA_API int ECMedia_GetShareWindows(void *handle, WindowShare ** windows);
/*
 ÂäüËÉΩ     : ËÆæÁΩÆÁõ¥Êí≠Êé®ÊµÅÁöÑËßÜÈ¢ëÊù•Ê∫ê
 ÂèÇÊï∞     : [IN]  handle	Ôºö Âè•ÊüÑ
 [		   [IN]  type	Ôºö 0 Ê°åÈù¢ 1 Á™óÂè£
 [IN]  id		Ôºö ÂØπ‰∫éÁöÑÊ°åÈù¢ÊàñËÄÖÁ™óÂè£id
 ËøîÂõûÂÄºÔºö Á™óÂè£‰∏™Êï∞
 */
ECMEDIA_API int ECMedia_SelectShareWindow(void *handle, int type , int id);
/*
 ÂäüËÉΩ     : ËÆæÁΩÆÁõ¥Êí≠Êé®ÊµÅÁöÑËßÜÈ¢ëÊù•Ê∫ê
 ÂèÇÊï∞     : [IN]  handle		Ôºö Âè•ÊüÑ
       [IN]  video_source			ÔºöËßÜÈ¢ëÊù•Ê∫ê 0 ÊëÑÂÉèÂ§¥ 1 ÊòØÊ°åÈù¢
 */
ECMEDIA_API void ECMedia_SetLiveVideoSource(void *handle, int video_source);
/*
 * ÂäüËÉΩÔºö ÂΩïÂà∂ÊëÑÂÉèÂ§¥ËßÜÈ¢ë‰øùÂ≠ò‰∏∫MP4Êñá‰ª∂
 * ÂèÇÊï∞Ôºö [IN] filename: MP4Â∞èËßÜÈ¢ëÊñá‰ª∂‰øùÂ≠òË∑ØÂæÑ
 *       [IN] localview: Ë¶ÅÁõ∏Êú∫È¢ÑËßàviewÁöÑÁà∂view
 */
ECMEDIA_API int ECMedia_startRecordLocalMedia(const char *fileName, void *localview);
/*
 * ÂÅúÊ≠¢ÂΩïÂà∂Â∞èËßÜÈ¢ë
 */
ECMEDIA_API void ECMedia_stopRecordLocalMedia();
/*
 * ËÆæÁΩÆRED
 */
ECMEDIA_API int ECMedia_setAudioRed(int channelid, bool enable, int payloadType);
/*
 *ÂäüËÉΩÔºöËÆæÁΩÆËßÜÈ¢ëÈÄöËÆØ‰∏≠ÂΩìÊú¨Âú∞ÊëÑÂÉèÂ§¥Êó†Ê≥ïÈááÈõÜËßÜÈ¢ëÊó∂ÔºåËøîÂõûÁªôÁî®Êà∑‰ø°ÊÅØÁöÑÂõûË∞ÉÂáΩÊï∞
 */
ECMEDIA_API int ECMedia_set_no_camera_capture_cb(int deviceid, onEcMediaNoCameraCaptureCb no_camera_capture_cb);
/*
 ÂäüËÉΩ		ÔºöËé∑ÂèñÁªüËÆ°Êä•Âëä
 ÂèÇÊï∞		Ôºö[IN] type: ÁªüËÆ°Êä•ÂëäÁ±ªÂûãÔºåËØ¶ÁªÜ‰ø°ÊÅØÊàñÂÖ≥ÈîÆ‰ø°ÊÅØ
 [OUT] reports: ÁªüËÆ°Êä•Âëä
 ËøîÂõûÂÄº   : ËøîÂõûÂÄº 0ÔºöÊàêÂäü„ÄÄ-1ÔºöÂ§±Ë¥•
 */
ECMEDIA_API int ECMedia_getStatsReports(int type, char* callid, void** pMediaStatisticsDataInnerArray, int *pArraySize);
ECMEDIA_API void ECMedia_deletePbData(void* mediaStatisticsDataInner);

/*
 * ÂÖàË∞ÉÁî®ECMedia_audio_set_magic_sound(channelid, 8, 0, 0)
 * ÂÜçË∞ÉÁî®ECMedia_audio_enable_magic_sound()
 * ‰ΩøËÉΩÂèòÂ£∞
 * channelid: channelid
 * is_enable: true: ÂêØÁî®Ôºåfalse: Á¶ÅÁî®
 */


ECMEDIA_API int ECMedia_audio_enable_magic_sound(int channelid, bool is_enable);

/*
 * ÂäüËÉΩÔºöËÆæÁΩÆÂèòÂ£∞ÂèÇÊï∞
 * channelID: channel id
 * pitch: Â£∞Ë∞ÉË∞ÉËäÇÔºàÂèòË∞É‰∏çÂèòÈÄüÔºâÔºåÂèñÂÄº[-12, 12]Ôºà0Ë°®Á§∫Ê≠£Â∏∏ÔºåÂéüÊù•Â£∞Èü≥Èü≥Ë∞ÉÔºåËÆæÁΩÆ‰∏∫+8ÁöÑËØùÔºåÂèØ‰ª•Âê¨Âà∞ÊòéÊòæÁöÑÂ•≥ÁîüÊïàÊûúÔºâ
 * tempo: Èü≥ÈÄüË∞ÉËäÇÔºàÂèòÈÄü‰∏çÂèòË∞ÉÔºâÔºåÂèñÂÄº[-50, 50], 0Ë°®Á§∫Ê≠£Â∏∏ÂéüËØ≠ÈÄü
 * rate:  ÂèòÈÄüÂèàÂèòË∞ÉÔºåÂèñÂÄº[-50, 50], 0Ë°®Á§∫Ê≠£Â∏∏Èü≥ÈÄüÂíåÈü≥Ë∞É
 */
ECMEDIA_API int ECMedia_audio_set_magic_sound(int channelid, int pitch, int tempo, int rate);
#ifdef __cplusplus
}
#endif

#endif /* defined(__servicecoreVideo__ECMedia__) */
