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
ECMEDIA_API int ECMedia_set_local_receiver(int channelid, int rtp_port, int rtcp_port = -1, bool ipv6 = false);
/*
 *1
 */
ECMEDIA_API int ECMedia_audio_set_send_destination(int channelid, int rtp_port, const char *rtp_addr, int source_port, int rtcp_port, const char *rtcp_ipaddr);
/**
 *
 */
/*
 *1 set audio ssrc, should be called immediately after audio channel is created
 */
ECMEDIA_API int ECMedia_audio_set_ssrc(int channelid, unsigned int localssrc, unsigned int remotessrc);

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
/*
*
*/
ECMEDIA_API int ECMedia_set_global_audio_in_device(bool enabled);
///*
// *
// */
//ECMEDIA_API int ECMedia_init_srtp(int channelid);
///*
// *加密
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
 *字符 语音通道
 */
ECMEDIA_API int ECMedia_send_dtmf(int channelid, const char dtmfch);
ECMEDIA_API int ECMedia_set_send_telephone_event_payload_type(int channelid, unsigned char type);
ECMEDIA_API int ECMedia_set_recv_telephone_event_payload_type(int channelid, unsigned char type);
/*
 *每次产生channel后传进来
 */
ECMEDIA_API int ECMedia_set_dtmf_cb(int channelid, onEcMediaReceivingDtmf dtmf_cb);
ECMEDIA_API int ECMedia_set_media_packet_timeout_cb(int channelid, onEcMediaPacketTimeout media_timeout_cb);
ECMEDIA_API int ECMedia_set_stun_cb(int channelid, onEcMediaStunPacket stun_cb);
ECMEDIA_API int ECMedia_set_audio_data_cb(int channelid, onEcMediaAudioData audio_data_cb);
/**
 * 设置音频 PCM 数据回调
 * @param channelid: channel id
 * @param callback : callback 具体解释见定义处
 * @return 成功返回0， 失败返回非0
 */
ECMEDIA_API int ECMedia_set_pcm_audio_data_cb(int channelid, cloopenwebrtc::ECMedia_PCMDataCallBack callback);
ECMEDIA_API int ECMedia_set_video_data_cb(int channelid, onEcMediaVideoDataV Video_data_cb);
ECMEDIA_API int ECMedia_set_voe_cb(int channelid, onVoeCallbackOnError voe_callback_cb);

/**
 * conference csrc callback
 * @param channelid: channel id
 * @param callback : callback @see ECMedia_ConferenceParticipantCallback
 * @return success return 0, eles return -1;
 */
ECMEDIA_API int ECMedia_setECMedia_ConferenceParticipantCallback(int channelid, cloopenwebrtc::ECMedia_ConferenceParticipantCallback* callback);
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
ECMEDIA_API int ECMedia_get_file_capture_capability(int capture_id, CameraCapability& capabilityp);
/*
*
*/
ECMEDIA_API int ECMedia_allocate_capture_file(int& deviceid, const char *fileUTF8, const char *filesSplit = nullptr);
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
 * 设置视频原始数据回调
 * @param channelid: channel id.
 * @param callback : 原始视频数据回调函数指针
 * @return 成功返回0，失败返回非0
 */
ECMEDIA_API int ECMedia_set_i420_framecallback(int channelid, cloopenwebrtc::ECMedia_I420FrameCallBack callback);

//add by dingxf
/**
* 设置远程视频原始数据回调
* @param channelid: channel id.
* @param callback : 远程视频数据回调函数指针
* @return 成功返回0，失败返回非0
*/
ECMEDIA_API int ECMedia_set_remote_i420_framecallback(int channelid, cloopenwebrtc::ECMedia_I420FrameCallBack callback);

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
 //桌面共享调用例子
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
    
/* switch h264 hard codec, only support ios
 * must call it after m_vie have create && befor video channel create.
 * @param encoder  true: enable h264 hard encode, false: disable.
 * @param decoder  true: enable h264 hard decode, false: disable.
 */
int ECMedia_iOS_h264_hard_codec_switch(bool encoder, bool decoder);
    
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
 * description: 设置底层yuv视频帧的缩放方式
 * FrameScaleType:
 *   kScaleTypeCropping -> 以裁剪方式缩放视频帧
 *   kScaleTypeFilling  -> 以填充方式缩放视频帧
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

// stream beauty face.
ECMEDIA_API int ECMedia_setBeautyFace(int deviceid, bool enable);

/**
 * set video filter for iOS
 * @param deviceid    device id
 * @param filter      video filter type for choosing.
 */
ECMEDIA_API int ECMedia_iOS_SetVideoFilter(int deviceid, cloopenwebrtc::ECImageFilterType filter);
/* 
// 播流接口调用流程
// 创建 handle
void *handle = ECMedia_createLiveStream();
// 设置预览view
ECMedia_setVideoPreviewViewer(handle, view);
// 开始播放
ECMedia_playLiveStream(handle, "http://livestream.com", callback);
// 停止播放
ECMedia_stopLiveStream(handle);
// 释放handle
ECMedia_releaseLiveStream(handle);
handle = NULL;

 
// 推流接口调用流程
void *handle = ECMedia_createLiveStream(0);
// 设置预览view
ECMedia_setVideoPreviewViewer(handle, view);
// 设置推流视频参数
ECMedia_ConfigLiveVideoStream(handle, config);
// 开始推流
ECMedia_pushLiveStream(handle, "http://livestream.com", callback);
// 切换前后置摄像头
ECMedia_SwitchLiveCamera(handle, camera_index);
// 停止推流
ECMedia_stopLiveStream(handle);
// 释放handle
ECMedia_releaseLiveStream(handle);
handle = NULL;
*/
    
/*
 功能     : 创建直播模块
 参数     : [IN]  type	  : 类型，必须为0
 返回值   : 返回值直播模块句柄
 */
ECMEDIA_API void *ECMedia_createLiveStream();
/*
 功能     : 开始观看直播
 参数     : [IN]  handle		： 句柄
 [IN]  url			 : 直播地址
 [IN]  renderView	：视频窗口
 [IN]  callback		：视频宽高回调
 返回值   : 返回值 0：成功  -1：初始化资源失败 -2：已经在直播或推流  -3：连接失败  -4：建立流失败
 */
ECMEDIA_API int ECMedia_playLiveStream(void *handle, const char * url, ECLiveStreamNetworkStatusCallBack callback);
/*
 功能     : 开始直播推流
 参数     : [IN]  handle		： 句柄
 [IN]  url			 : 推流地址
 [IN]  renderView	：本地视频窗口
 返回值   : 返回值 0：成功　-1：初始化资源失败 -2：已经在直播或者推流  -3：连接失败  -4：建立流失败
 */
ECMEDIA_API int ECMedia_pushLiveStream(void *handle, const char *url, ECLiveStreamNetworkStatusCallBack callback);
/*
 功能     : 停止观看或推流
 参数     :	  [IN]  handle		： 句柄
 */
ECMEDIA_API void ECMedia_stopLiveStream(void *handle);
/*
 功能     : 释放直播模块
 参数     :	  [IN]  handle		： 句柄
 */
ECMEDIA_API void ECMedia_releaseLiveStream(void *handle);

/*
 * 配置推流视频参数
 * config: 视频参数配置，具体解释见 LiveVideoStreamConfig 定义处
 */
ECMEDIA_API int ECMedia_ConfigLiveVideoStream(void *handle, LiveVideoStreamConfig config);

/**
 * 旋转推流画面角度
 * degree: 旋转角度
 */
ECMEDIA_API int ECMedia_setLiveVideoFrameDegree(void *handle, ECLiveFrameDegree degree);
    
/**
 * 设置视频预览view
 */
ECMEDIA_API int ECMedia_setVideoPreviewViewer(void *handle, void *view);


/**
* 设置水印，图片或者文本
* @param deviceid deviceid
* @param watermark 构造水印参数
* @param width height capture的图像高和宽
* @return  uccess 0, errno -99;
*/
//add by chwd
ECMEDIA_API int ECMedia_set_watermark(int deviceid, cloopenwebrtc::WaterMark watermark, int width, int height);


/**
 *
 * @param camera_index
 * @return
 */
ECMEDIA_API int ECMedia_SwitchLiveCamera(void *handle, int camera_index);

/*
 功能     : 设置直播推流的视频来源
 参数     : [IN]   handle	： 句柄
       [OUT]  windows	：windows 数组指针
       返回值： 窗口个数
 */
ECMEDIA_API int ECMedia_GetShareWindows(void *handle, WindowShare ** windows);
/*
 功能     : 设置直播推流的视频来源
 参数     : [IN]  handle	： 句柄
 [		   [IN]  type	： 0 桌面 1 窗口
 [IN]  id		： 对于的桌面或者窗口id
 返回值： 窗口个数
 */
ECMEDIA_API int ECMedia_SelectShareWindow(void *handle, int type , int id);


/*
 功能     : 设置直播推流的视频来源
 参数     : [IN]  handle		： 句柄
       [IN]  video_source			：视频来源 0 摄像头 1 是桌面
 */
ECMEDIA_API void ECMedia_SetLiveVideoSource(void *handle, int video_source);
    
/*
 功能     : 开始直播模块美颜
 参数     :	  [IN]  handle		： 句柄
 */
ECMEDIA_API void ECMedia_enableLiveStreamBeauty(void *handle);
/*
 功能     : 停止直播模块美颜
 参数     :	  [IN]  handle		： 句柄
 */
ECMEDIA_API void ECMedia_disableLiveStreamBeauty(void *handle);
    
/*
 * 功能： 录制摄像头视频保存为MP4文件
 * 参数： [IN] filename: MP4小视频文件保存路径
 *       [IN] localview: 要相机预览view的父view
 */
ECMEDIA_API int ECMedia_startRecordLocalMedia(const char *fileName, void *localview);
/*
 * 停止录制小视频
 */
ECMEDIA_API void ECMedia_stopRecordLocalMedia();
/*
 * 设置RED
 */
ECMEDIA_API int ECMedia_setAudioRed(int channelid, bool enable, int payloadType);
/*
 *功能：设置视频通讯中当本地摄像头无法采集视频时，返回给用户信息的回调函数
 */
ECMEDIA_API int ECMedia_set_no_camera_capture_cb(int deviceid, onEcMediaNoCameraCaptureCb no_camera_capture_cb);
/*
 功能		：获取统计报告
 参数		：[IN] type: 统计报告类型，详细信息或关键信息
 [OUT] reports: 统计报告
 返回值   : 返回值 0：成功　-1：失败
 */
ECMEDIA_API int ECMedia_getStatsReports(int type, char* callid, void** pMediaStatisticsDataInnerArray, int *pArraySize);
ECMEDIA_API void ECMedia_deletePbData(void* mediaStatisticsDataInner);

/*
 * 先调用ECMedia_audio_set_magic_sound(channelid, 8, 0, 0)
 * 再调用ECMedia_audio_enable_magic_sound()
 * 使能变声
 * channelid: channelid
 * is_enable: true: 启用，false: 禁用
 */
ECMEDIA_API int ECMedia_audio_enable_magic_sound(int channelid, bool is_enable);

/*
 * 功能：设置变声参数
 * channelID: channel id
 * pitch: 声调调节（变调不变速），取值[-12, 12]（0表示正常，原来声音音调，设置为+8的话，可以听到明显的女生效果）
 * tempo: 音速调节（变速不变调），取值[-50, 50], 0表示正常原语速
 * rate:  变速又变调，取值[-50, 50], 0表示正常音速和音调
 */
ECMEDIA_API int ECMedia_audio_set_magic_sound(int channelid, int pitch, int tempo, int rate);
/*
 * 功能：设置变声模式
 * channelID: channel id
 * mode: kECMagicSoundNormal, kECMagicSoundHigh, kECMagicSoundLow
 */
ECMEDIA_API int ECMedia_select_magic_sound_mode(int channelid, cloopenwebrtc::ECMagicSoundMode mode);
    
/*
 * 功能：声音播放前进行放大
 * channelID: channel id
 * gain: 放大的倍数，1.5即将原来的声音放大1.5倍。
 */
ECMEDIA_API int ECMedia_audio_set_playout_gain(int channelid, float gain);
/*
 * 功能：把microphone采集的声音进行放大
 * channelID: channel id
 * gain: 放大的倍数，1.5即将原来的声音放大1.5倍。
 */
ECMEDIA_API int ECMedia_audio_set_microphone_gain(int channelid, float gain);

ECMEDIA_API int ECMedia_audio_set_mix_mediastream(int channel, bool enable, char *mixture, unsigned char version);

ECMEDIA_API int ECMedia_video_set_mix_mediastream(int channel, bool enable, char *mixture, unsigned char version);

#ifdef __cplusplus
}
#endif

#endif /* defined(__servicecoreVideo__ECMedia__) */
