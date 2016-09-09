#include "servicecore.h"
#include <ctype.h>
#include "serphonecall.h"
#include "friends.h"
#include "lpconfig.h"
#include "enum.h"
#include "sometools.h"
#include "serprivate.h"
#include "salpr.h"

#include "eXosip2.h"
#include <eXosip2/eXosip.h>
#ifdef WIN32
#include "codingHelper.h"
#endif

#if !defined(NO_VOIP_FUNCTION)
#include "ECMedia.h"
#endif

int voe_callback(int channel, int errCode) {

	PrintConsole("voe_callback channid=%d, errCode=%d", channel, errCode);
	return 0;
}

void ServiceCore::ring_stop(int ringmode)
{
#if !defined(NO_VOIP_FUNCTION)

	int stop_channel_id = -1;
	if (0 == ringmode) {
		stop_channel_id = local_playfile_channelID;
	}
	else if (1 == ringmode)
	{
		stop_channel_id = local_playfile_channelID_prering;
	}
	else
	{
		stop_channel_id = local_playfile_channelID_afterring;
	}
	if ( stop_channel_id >=0 )
	{
		PrintConsole("ServiceCore::ring_stop(),ringmode=%d,channelID=%d\n",ringmode,stop_channel_id);
		ECMedia_ring_stop(stop_channel_id);
		if (0 == ringmode) {
			local_playfile_channelID = -1;
		}
		else if (1 == ringmode)
		{
			local_playfile_channelID_prering = -1;
		}
		else
		{
			local_playfile_channelID_afterring = -1;
		}
	}
#endif
	return;
}

int  ServiceCore::ring_start(const char *filename, int interval, int ringmode)
{
#if !defined(NO_VOIP_FUNCTION)
	if (0 == ringmode) {
		ECMedia_audio_create_channel(local_playfile_channelID, false);
	}	else if (1 == ringmode)	{
		ECMedia_audio_create_channel(local_playfile_channelID_prering, false);
	}	else	{
		ECMedia_audio_create_channel(local_playfile_channelID_afterring, false);
	}
	int ret = -1;
	if (0 == ringmode) {
		ret = ECMedia_ring_start(local_playfile_channelID, filename, true);
	}	else if (1 == ringmode) { //prering	
		ret = ECMedia_ring_start(local_playfile_channelID_prering, filename, true);
	}	else  { //afterring	
		ret = ECMedia_ring_start(local_playfile_channelID_afterring, filename, true);
	}
	return ret;
#endif
	return 1;
}

void ServiceCore::serphone_core_play_tone()
{
}

void ServiceCore::serphone_core_update_streams_destinations(SerPhoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md) {

#if !defined(NO_VOIP_FUNCTION)
	SalStreamDescription *old_audiodesc = NULL;
	SalStreamDescription *old_videodesc = NULL;
	SalStreamDescription *new_audiodesc = NULL;
	SalStreamDescription *new_videodesc = NULL;

	char *rtp_addr, *rtcp_addr;
	int i;

	for (i = 0; i < old_md->n_active_streams; i++) {
		if (old_md->streams[i].type == SalAudio) {
			old_audiodesc = &old_md->streams[i];
		} else if (old_md->streams[i].type == SalVideo) {
			old_videodesc = &old_md->streams[i];
		}
	}
	for (i = 0; i < new_md->n_active_streams; i++) {
		if (new_md->streams[i].type == SalAudio) {
			new_audiodesc = &new_md->streams[i];
		} else if (new_md->streams[i].type == SalVideo) {
			new_videodesc = &new_md->streams[i];
		}
	}
	if (call->audiostream && new_audiodesc) {
		rtp_addr = (new_audiodesc->rtp_addr[0] != '\0') ? new_audiodesc->rtp_addr : new_md->addr;
		rtcp_addr = (new_audiodesc->rtcp_addr[0] != '\0') ? new_audiodesc->rtcp_addr : new_md->addr;
		PrintConsole("Change audio stream destination: RTP=%s:%d RTCP=%s:%d\n", rtp_addr, new_audiodesc->rtp_port, rtcp_addr, new_audiodesc->rtcp_port);

		int change_audio = ECMedia_audio_set_send_destination(call->m_AudioChannelID, new_audiodesc->rtp_port, rtp_addr,-1,new_audiodesc->rtcp_port);
//        int change_audio = base->SetSendDestination(call->m_AudioChannelID, 7078, "127.0.0.1",kVoEDefault, 7079);

		if (0 == change_audio && SalStreamSendRecv == new_audiodesc->dir) {
			serphone_call_set_state(call, LinphoneCallUpdatedAudioDestinationChanged, rtp_addr);
		}
	}

#ifdef VIDEO_ENABLED
	if (call->videostream && new_videodesc) {
		rtp_addr = (new_videodesc->rtp_addr[0] != '\0') ? new_videodesc->rtp_addr : new_md->addr;
		rtcp_addr = (new_videodesc->rtcp_addr[0] != '\0') ? new_videodesc->rtcp_addr : new_md->addr;
		PrintConsole("Change video stream destination: RTP=%s:%d RTCP=%s:%d\n", rtp_addr, new_videodesc->rtp_port, rtcp_addr, new_videodesc->rtcp_port);

		int change_video = ECMedia_video_set_send_destination(call->m_VideoChannelID, rtp_addr,new_videodesc->rtp_port,new_videodesc->rtcp_port);
		if (0 == change_video && SalStreamSendRecv == new_videodesc->dir) {
			serphone_call_set_state(call, LinphoneCallUpdatedVideoDestinationChanged, rtp_addr);

			cloopenwebrtc::VideoCodec tempVideoCodec;
			ECMedia_get_send_codec_video(call->m_VideoChannelID, tempVideoCodec);
			ECMedia_set_send_codec_video(call->m_VideoChannelID, tempVideoCodec);
			ECMedia_set_receive_codec_video(call->m_VideoChannelID,tempVideoCodec);
		}
	}
#endif

	/* Copy address and port values from new_md to old_md since we will keep old_md as resultdesc */
	strcpy(old_md->addr, new_md->addr);
	if (old_audiodesc && new_audiodesc) {
		strcpy(old_audiodesc->rtp_addr, new_audiodesc->rtp_addr);
		strcpy(old_audiodesc->rtcp_addr, new_audiodesc->rtcp_addr);
		old_audiodesc->port = new_audiodesc->port;
		old_audiodesc->rtp_port = new_audiodesc->rtp_port;
		old_audiodesc->rtcp_port = new_audiodesc->rtcp_port;
	}
	if (old_videodesc && new_videodesc) {
		strcpy(old_videodesc->rtp_addr, new_videodesc->rtp_addr);
		strcpy(old_videodesc->rtcp_addr, new_videodesc->rtcp_addr);
		old_videodesc->port = new_videodesc->port;
		old_videodesc->rtp_port = new_videodesc->rtp_port;
		old_videodesc->rtcp_port = new_videodesc->rtcp_port;
	}
#endif
}

void ServiceCore::serphone_core_update_streams(SerPhoneCall *call, SalMediaDescription *new_md)
{
#if !defined(NO_VOIP_FUNCTION)
	SalMediaDescription *oldmd=call->resultdesc;

	//add by xzq to trace the stream
	if( new_md!= NULL) {
		PrintConsole("Media stream is  R[%s:%d] <-->L[%s:%d]\n",new_md->addr, new_md->streams[0].port,
			call->localdesc->addr,call->audio_port);
	}
	if ( m_ringplay_flag ){
		ring_stop();
		m_ringplay_flag = FALSE;
		dmfs_playing_start_time = 0;
	}
	if (new_md!=NULL){
		sal_media_description_ref(new_md);
		call->media_pending=FALSE;
	}else{
		call->media_pending=TRUE;
	}
	call->resultdesc=new_md;
	if (call->m_AudioChannelID>=0 || call->m_VideoChannelID>=0){
		// we already started media: check if we really need to restart it
		if (oldmd){
			int md_changed = media_parameters_changed(call, oldmd, new_md);
			if ((md_changed & SAL_MEDIA_DESCRIPTION_CODEC_CHANGED) || call->playing_ringbacktone) {
				PrintConsole("Media descriptions are different, need to restart the streams.\n");

			} else if (md_changed == SAL_MEDIA_DESCRIPTION_UNCHANGED) {
				call->resultdesc=oldmd;
				sal_media_description_unref(&new_md);
				if (call->all_muted) {
					PrintConsole("Early media finished, unmuting inputs...\n");
					//we were in early media, now we want to enable real media
					serphone_call_enable_camera (call,serphone_call_camera_enabled (call));
					if (call->m_AudioChannelID >=0)
						serphone_core_mute_mic ( serphone_core_is_mic_muted());
#ifdef VIDEO_ENABLED
					/*if (call->videostream && call->camera_active)
					video_stream_change_camera(call->videostream,lc->video_conf.device );*/
#endif
				}
				PrintConsole("No need to restart streams, SDP is unchanged.\n");
				return;

			} else {
				if ( (md_changed & SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED) ) {
					PrintConsole("Network parameters have changed, update them.\n");
					serphone_core_update_streams_destinations(call, oldmd, new_md);
				}
				call->resultdesc = oldmd;
				sal_media_description_unref(&new_md);
				return;
			}
		}
		serphone_call_stop_media_streams (call);
		serphone_call_init_media_streams (call);
	}
	if (oldmd)
		sal_media_description_unref(&oldmd);

	if (new_md) {
		bool_t all_muted=FALSE;
		bool_t send_ringbacktone=FALSE;

		if (call->m_AudioChannelID < 0){
			//this happens after pausing the call locally. The streams is destroyed and then we wait the 200Ok to recreate it
			serphone_call_init_media_streams (call);
		}
		if (call->state==LinphoneCallIncomingEarlyMedia && serphone_core_get_remote_ringback_tone ()!=NULL){
			send_ringbacktone=TRUE;
		}
		if (call->state==LinphoneCallIncomingEarlyMedia ||
			(call->state==LinphoneCallOutgoingEarlyMedia && !call->params.real_early_media)){
				all_muted=TRUE;
		}
		serphone_call_start_media_streams(call,all_muted,send_ringbacktone);
	}
#endif
}

void ServiceCore::serphone_call_stop_media_streams(SerPhoneCall *call)
{
#if !defined(NO_VOIP_FUNCTION)
	if (call->m_AudioChannelID>=0) {
		//getCallStatistics(0 ,&m_lastCallStatisticsInfo);
		audio_stream_stop(call->m_AudioChannelID);
		call->m_AudioChannelID = -1;
	}

#ifdef VIDEO_ENABLED
	if (m_videoModeChoose == 0) //real-time
	{
		if (call->m_VideoChannelID>=0){
			video_stream_stop(call->m_VideoChannelID, call->m_CaptureDeviceId);
            call->m_VideoChannelID=-1;
            call->m_CaptureDeviceId = -1;
//            video_stream_stop(call->m_VideoChannelID1, call->m_CaptureDeviceId);
//            call->m_VideoChannelID1 = -1;
//            video_stream_stop(call->m_VideoChannelID2, call->m_CaptureDeviceId);
//            call->m_VideoChannelID2 = -1;
			
		}
	}else if (m_videoModeChoose == 1) //screen-share
	{
		if (call->m_VideoChannelID>=0){
			video_stream_stop(call->m_VideoChannelID, m_desktopCaptureId);
			call->m_VideoChannelID=-1;
			m_desktopCaptureId = -1;
		}
	}
	
#endif

	if (call->audio_profile){
		rtp_profile_clear_all(call->audio_profile);
		rtp_profile_destroy(call->audio_profile);
		call->audio_profile=NULL;
	}

	if (call->video_profile){
		rtp_profile_clear_all(call->video_profile);
		rtp_profile_destroy(call->video_profile);
		call->video_profile=NULL;
	}
#endif
    ECMedia_stop_Statistics_proxy();
}

void ServiceCore::media_stream_free(MediaStream *stream) {
	if (stream->session != NULL) {
		rtp_session_unregister_event_queue(stream->session, stream->evq);
		rtp_session_destroy(stream->session);
		stream->session = NULL;
	}
	if (stream->evq) ortp_ev_queue_destroy(stream->evq);
}

void ServiceCore::audio_stream_free(AudioStream *stream) {
	media_stream_free(&stream->ms);
	ms_free((void **)&stream);
}

void ServiceCore::audio_stream_stop(int channelID)
{
#if !defined(NO_VOIP_FUNCTION)
	if (channelID>=0) {
		if (srtp_enable) {
			if( ECMedia_shutdown_srtp(channelID) ) {
				PrintConsole("Remove SRTP fail code\n");
			}
		}

		MSList *calls;
		SerPhoneCall *call;
		calls= this->calls;
		while(calls!= NULL){
			call = (SerPhoneCall *)calls->data;
			/* get immediately a reference to next one in case the one
			we are going to examine is destroy and removed during
			linphone_core_start_invite() */
			if (call->m_AudioChannelID == channelID && call->audiostream != NULL) {
				if (call->audiostream->ms.ice_check_list != NULL) {
					ice_check_list_print_route(call->audiostream->ms.ice_check_list, "Audio session's route");
					call->audiostream->ms.ice_check_list = NULL;
				}

				rtp_session_unregister_event_queue(call->audiostream->ms.session,call->audiostream_app_evq);
				ortp_ev_queue_flush(call->audiostream_app_evq);
				ortp_ev_queue_destroy(call->audiostream_app_evq);
				call->audiostream_app_evq=NULL;

				audio_stream_free(call->audiostream);
				call->audiostream=NULL;
				call->m_AudioChannelID = -1;
				break;
			}

			calls=calls->next;
		}

		ECMedia_DeRegister_voice_engine_observer();
		ECMedia_audio_stop_playout(channelID);
	    ECMedia_audio_stop_receive(channelID);
		ECMedia_audio_stop_send(channelID);
        ECMedia_audio_stop_record();
		ECMedia_delete_channel(channelID, false);
	}
#ifndef WIN32
	ECMedia_uninit_audio();
#endif
#endif
}

void ServiceCore::video_stream_free(VideoStream *stream) {
	media_stream_free(&stream->ms);
	if (stream->display_name!=NULL)
	{
		ms_free((void **)&stream->display_name);
	}
	ms_free ((void **)&stream);
}

void ServiceCore::video_stream_stop(int channelID,int captureID)
{
#ifdef VIDEO_ENABLED
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	{
#ifdef	ENABLE_RECORD_RAW_VIDEO
		/*ViEFile *file_record = ViEFile::GetInterface(m_vie);
		file_record->StopRecordIncomingVideo(channelID);
		file_record->StopRecordOutgoingVideo(channelID);
		file_record->Release();*/
		ViECodec *vie_codec = ViECodec::GetInterface(m_vie);
		if (vie_codec)
		{
			vie_codec->StopDebugRecording(channelID);
			vie_codec->Release();
		}
#endif
	}
	if (m_videoModeChoose == 0) //real-time
	{
		//TODO:
		//ViECapture *capture = ViECapture::GetInterface(m_vie);
		//capture->DeregisterObserver(captureID);
		ECMedia_stop_capture(captureID);

	}else if(m_videoModeChoose == 1) //screen-share
	{
		ECMedia_release_desktop_capture(captureID);
	}

	MSList *calls;
	SerPhoneCall *call;
	calls= this->calls;
	while(calls!= NULL){
		call = (SerPhoneCall *)calls->data;
		/* get immediately a reference to next one in case the one
		we are going to examine is destroy and removed during
		linphone_core_start_invite() */
		if (call->m_VideoChannelID == channelID) {
			//                call->m_VideoChannelID = -1;
			if (call->videostream != NULL) {
				rtp_session_unregister_event_queue(call->videostream->ms.session,call->videostream_app_evq);
				ortp_ev_queue_flush(call->videostream_app_evq);
				ortp_ev_queue_destroy(call->videostream_app_evq);
				call->videostream_app_evq=NULL;
				if (call->videostream->ms.ice_check_list != NULL) {
					ice_check_list_print_route(call->videostream->ms.ice_check_list, "Video session's route");
					//                        ice_check_list_destroy(call->videostream->ms.ice_check_list);
					call->videostream->ms.ice_check_list = NULL;
				}
				video_stream_free(call->videostream);
				call->videostream=NULL;
				break;
			}
		}
		calls=calls->next;
	}        

	ECMedia_stop_render(channelID, captureID);
	ECMedia_video_stop_receive(channelID);
    ECMedia_video_stop_send(channelID);
	ECMedia_delete_channel(channelID, true);
#endif
}

int ServiceCore::return_video_width_height(int width,int height,int videoChannelID)
{
	PrintConsole("[DEBUG] %s videoChannelID:%d,width:%d, height:%d\n",__FUNCTION__,videoChannelID,width,height);

#ifdef VIDEO_ENABLED

	//update receive codec.
	cloopenwebrtc::VideoCodec codec_params;
	ECMedia_get_receive_codec_video(videoChannelID, codec_params);
	codec_params.width = width;
	codec_params.height = height;
	ECMedia_set_receive_codec_video(videoChannelID, codec_params);

	std::map<int,VideoConferenceDesc *>::iterator itt = videoConferenceM.find(videoChannelID);
	if (vtable.remote_video_ratio_changed) {
		if (itt != videoConferenceM.end()) {
			vtable.remote_video_ratio_changed(NULL, width, height, true,itt->second->remoteSip);
		} else {
			vtable.remote_video_ratio_changed(NULL, width, height,false, NULL);
		}
	}
#endif
	return 0;
}

void ServiceCore::serphone_call_start_media_streams(SerPhoneCall *call, bool_t all_inputs_muted,
	bool_t send_ringbacktone)
{
#if !defined(NO_VOIP_FUNCTION)
	call->current_params.audio_codec = NULL;
	call->current_params.video_codec = NULL;

	SerphoneAddress *me=serphone_core_get_primary_contact_parsed();
	char *cname;
	bool_t use_arc=serphone_core_adaptive_rate_control_enabled();
#ifdef VIDEO_ENABLED
	const SalStreamDescription *vstream=sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpAvp,SalVideo);
#endif

	if(call->m_AudioChannelID < 0)
	{
		PrintConsole("start_media_stream() called without prior init !\n");
		return;
	}
	cname=serphone_address_as_string_uri_only(me);

#if defined(VIDEO_ENABLED)
	if (vstream!=NULL && vstream->dir!=SalStreamInactive && vstream->payloads!=NULL){
		/*when video is used, do not make adaptive rate control on audio, it is stupid.*/
		use_arc=FALSE;
	}
#endif
	serphone_call_start_audio_stream(call,cname,all_inputs_muted,send_ringbacktone,use_arc); 
	if (call->m_VideoChannelID >=0 ) {
		serphone_call_start_video_stream(call,cname,all_inputs_muted);
		call->current_params.has_video = true;
	}
	else
		call->current_params.has_video= false;
#if defined(VIDEO_ENABLED)
    if(call->m_VideoChannelID >= 0) {
        ECMedia_set_video_SendStatistics_proxy(call->m_VideoChannelID, "/storage/emulated/0/Statistics.log", 1000);
        ECMedia_set_video_RecvStatistics_proxy(call->m_VideoChannelID, "/storage/emulated/0/Statistics.log", 1000);
    } 
    else
#endif
    {
        ECMedia_set_audio_RecvStatistics_proxy(call->m_AudioChannelID, "/storage/emulated/0/Statistics.log", 1000);
        ECMedia_set_audio_SendStatistics_proxy(call->m_AudioChannelID, "/storage/emulated/0/Statistics.log", 1000);
    }

        
	call->all_muted=all_inputs_muted;
	call->playing_ringbacktone=send_ringbacktone;
	call->up_bw=serphone_core_get_upload_bandwidth();
	if (srtp_enable) {
		call->current_params.media_encryption = LinphoneMediaEncryptionSRTP;
	}

	/*also reflect the change if the "wished" params, in order to avoid to propose SAVP or video again
	* further in the call, for example during pause,resume, conferencing reINVITEs*/
	serphone_call_fix_call_parameters(call);
	if ((call->ice_session != NULL) && (ice_session_state(call->ice_session) != IS_Completed)) {
		ice_session_start_connectivity_checks(call->ice_session);
	}
	goto end;
end:
	ms_free((void **)&cname);
	serphone_address_destroy(me);
#endif
}

void ServiceCore::serphone_call_start_audio_stream(SerPhoneCall *call, const char *cname,
	bool_t muted, bool_t send_ringbacktone, bool_t use_arc)
{
#if !defined(NO_VOIP_FUNCTION)
#ifndef WIN32
	ECMedia_init_audio();
#endif
	PrintConsole("cloopen trace %s begin\n",__FUNCTION__);
	int used_pt=-1;
	/* look for savp stream first */
	const SalStreamDescription *stream=sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpSavp,SalAudio);
	const SalStreamDescription *local_stream = sal_media_description_find_stream(call->localdesc,
		SalProtoRtpSavp,SalAudio);

	/* no savp audio stream, use avp */
	if (!stream)
		stream=sal_media_description_find_stream(call->resultdesc, SalProtoRtpAvp,SalAudio);
	if (!local_stream)
		local_stream = sal_media_description_find_stream(call->localdesc, SalProtoRtpAvp,SalAudio);

	if (stream && stream->dir!=SalStreamInactive && stream->port!=0){
		call->audio_profile=make_profile(call,call->resultdesc,stream,&used_pt);

		if (used_pt!=-1){
			call->current_params.audio_codec = rtp_profile_get_payload(call->audio_profile, used_pt);
			PayloadType *p=call->current_params.audio_codec;

			if (srtp_enable) {
				char master_key[65];
				//Here we should use b64_encode to encode key
				generate_b64_crypto_key(46, master_key, (const char *)key);
				PrintConsole("Here user_mode = %d, before func EnableSRTPReceive and EnableSRTPSend we check master key send = %s,stream->crypto[0].master_key receive = %s, which is after b64_encode\n",user_mode,master_key,stream->crypto[0].master_key);
			
				ECMedia_enable_srtp_receive(call->m_AudioChannelID, reinterpret_cast<const char *> (user_mode?master_key:stream->crypto[0].master_key));
				ECMedia_enable_srtp_send(call->m_AudioChannelID, reinterpret_cast<const char *>(master_key));

				//encryptType = stream->crypto[0].algo;
				//switch (encryptType) {
				//case CCPAES_256_SHA1_80:
				//	{
				//		encrypt->EnableSRTPReceive(call->m_AudioChannelID, kCipherAes256CounterMode, 64, kAuthHmacSha1, 0, kAuthTagLength80, kEncryptionAndAuthentication,reinterpret_cast<const unsigned char *> (user_mode?master_key:stream->crypto[0].master_key) );

				//		encrypt->EnableSRTPSend(call->m_AudioChannelID, kCipherAes256CounterMode, 64, kAuthHmacSha1, 0, kAuthTagLength80, kEncryptionAndAuthentication,reinterpret_cast<const unsigned char *>(master_key),0 );
				//	}
				//	break;
				//case CCPAES_256_SHA1_32:
				//	{
				//		encrypt->EnableSRTPReceive(call->m_AudioChannelID, kCipherAes256CounterMode, 64, kAuthHmacSha1, 0, kAuthTagLength32, kEncryptionAndAuthentication,reinterpret_cast<const unsigned char *> (user_mode?master_key:stream->crypto[0].master_key) );

				//		encrypt->EnableSRTPSend(call->m_AudioChannelID, kCipherAes256CounterMode, 64, kAuthHmacSha1, 0, kAuthTagLength32, kEncryptionAndAuthentication,reinterpret_cast<const unsigned char *>(master_key),0 );
				//	}
				//	break;
				//default:
				//	break;
				//}
			}

			ECMedia_audio_set_send_destination(call->m_AudioChannelID, stream->port,stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr);
//            For MOS test
//			ECMedia_audio_set_send_destination(call->m_AudioChannelID, 7078, "127.0.0.1");

			//add by xzq to trace the stream
			PrintConsole("Send Stream to Remote [%s:%d]\n",stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr,
				stream->port);
			cloopenwebrtc::CodecInst codec_params = {0};
			bool codec_found = false;
			int codec_num = ECMedia_num_of_supported_codecs_audio();
			cloopenwebrtc::CodecInst *codecArray = new cloopenwebrtc::CodecInst[codec_num];
			ECMedia_get_supported_codecs_audio(codecArray);

			for (int i = 0; i < codec_num; i++) {
				codec_params = codecArray[i];
				if ( strcasecmp( codec_params.plname,p->mime_type ) == 0 &&
					codec_params.plfreq == p->clock_rate ) {

						codec_found = true;
						codec_params.pltype = used_pt;

						if(m_packetInterval > 0)
						{
							codec_params.pacsize = codec_params.plfreq*m_packetInterval/1000;

							if(!strcasecmp(codec_params.plname, "ILBC"))
							{
								if(codec_params.pacsize == 240 || codec_params.pacsize == 480)
									codec_params.rate = 13300;
								else
									codec_params.rate = 15200;
							}
						}

#ifdef _WIN32
						m_silkBitRate = 20000;
#endif
						if(!strcasecmp(codec_params.plname, "SILK") && (m_silkBitRate > 0))
						{
							codec_params.rate = m_silkBitRate;
						}

						break;
				}
			}
			delete []codecArray;

			if (codec_found) {
				//add by xzq to trace the media
				PrintConsole("cloopen trace %s middle 111\n",__FUNCTION__);
				PrintConsole("Codec is : playload type = %d, payload name is %s  \n",
					codec_params.pltype, codec_params.plname);

                codec_params.fecEnabled = m_enable_fec;

				ECMedia_set_send_codec_audio(call->m_AudioChannelID, codec_params);
				ECMedia_set_receive_playloadType_audio(call->m_AudioChannelID,codec_params);

				//TODO:
                //base->SetFecStatus(call->m_AudioChannelID, m_enable_fec);
                //base->SetLoss(call->m_AudioChannelID, m_opus_packet_loss_rate);


				//for fec test.
/*				cloopenwebrtc::CodecInst redCodec = codec_params;
				memset(redCodec.plname, 0, RTP_PAYLOAD_NAME_SIZE);
				memcpy(redCodec.plname, "red", 3);
				redCodec.pltype = 116;
				ECMedia_set_receive_playloadType_audio(call->m_AudioChannelID,redCodec);

				cloopenwebrtc::CodecInst fecCodec = codec_params;
				memset(fecCodec.plname, 0, RTP_PAYLOAD_NAME_SIZE);
				memcpy(fecCodec.plname, "ulpfec", 6);
				fecCodec.pltype = 117;
				ECMedia_set_receive_playloadType_audio(call->m_AudioChannelID,fecCodec);	*/				
				//TODO:
				//rtp_rtcp->SetFECStatus(call->m_AudioChannelID, true, 116, 117);

				ECMedia_set_VAD_status(call->m_AudioChannelID, cloopenwebrtc::kVadAggressiveHigh, !m_dtxEnabled);
			}
			else {
				PrintConsole("Can't find codec,mime(%s),clock(%d)\n",p->mime_type,p->clock_rate);
			}

			if(stream->nack_support)
			{
				ECMedia_set_NACK_status(call->m_AudioChannelID, false);
			}
			ECMedia_set_RTCP_status(call->m_AudioChannelID);
			//rtp_rtcp->SetFECStatus(call->m_AudioChannelID,  true);

			PrintConsole("cloopen trace %s middle 112\n",__FUNCTION__);
			if(m_usedSpeakerIndex >= 0)
				ECMedia_select_playout_device(m_usedSpeakerIndex);
			if(m_usedMicrophoneIndex >= 0)
				ECMedia_select_record_device(m_usedMicrophoneIndex);
			//hardware->SetPlayoutDevice(0);
			PrintConsole("cloopen trace %s middle 113.\n",__FUNCTION__);
			if ( local_stream){
				switch(local_stream->dir)
				{
				case SalStreamSendOnly:                    
					ECMedia_audio_start_send(call->m_AudioChannelID);
                    ECMedia_audio_start_record();
					break;
				case SalStreamRecvOnly:
					ECMedia_audio_start_receive(call->m_AudioChannelID);
					ECMedia_audio_start_playout(call->m_AudioChannelID);
					break;
				case SalStreamInactive:
					break;
				default:
					ECMedia_audio_start_receive(call->m_AudioChannelID);
					ECMedia_audio_start_playout(call->m_AudioChannelID);
					ECMedia_audio_start_send(call->m_AudioChannelID);
                    ECMedia_audio_start_record();
					break;
				}
			}
			else{
				ECMedia_audio_start_receive(call->m_AudioChannelID);
				ECMedia_audio_start_playout(call->m_AudioChannelID);
				ECMedia_audio_start_send(call->m_AudioChannelID);
                ECMedia_audio_start_record();
			}
			PrintConsole("cloopen trace %s middle 114\n",__FUNCTION__);

			//TODO:
			//bool enabled = false;
			//int timeout = 0;
			/*if(network)
			network->GetPacketTimeoutNotification(call->m_AudioChannelID, enabled, timeout);*/
			//if(enabled) {
			//	if(!call->voe_observer) {
			//		call->voe_observer = new VoeObserver(call);
			//	}
			//	base->RegisterVoiceEngineObserver(*call->voe_observer);
			//}

			//TODO:
			//if(call->record_voip && (call->record_voip->isStartRecordMp4() || call->record_voip->isStartRecordWav())) {
			//	PrintConsole("RegisterExternalMediaProcessin in serphone_call_start_audio_stream\n");
			//	VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
			//	if(exmedia) {
			//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kPlaybackPerChannel, *call->record_voip);
			//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kRecordingPerChannel, *call->record_voip);
			//		exmedia->Release();
			//	}
			//}
			PrintConsole("cloopen trace %s middle 115\n",__FUNCTION__);
			call->current_params.in_conference=call->params.in_conference;

			ECMedia_set_voe_cb(call->m_AudioChannelID, voe_callback);
		}else PrintConsole("No audio stream accepted ?\n");
	}
#endif
	PrintConsole("cloopen trace %s end\n",__FUNCTION__);
}


void ServiceCore::serphone_call_start_video_stream(SerPhoneCall *call, const char *cname,bool_t all_inputs_muted)
{ 
#ifdef VIDEO_ENABLED
	if (m_SnapshotChannelID>=0) {
		stopVideoWithoutCall();
	}

	int used_pt=-1;
	/* look for savp stream first */
	const SalStreamDescription *stream = sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpSavp,SalVideo);
	const SalStreamDescription *local_stream = sal_media_description_find_stream(call->localdesc,
		SalProtoRtpSavp,SalVideo);

	/* no savp video stream, use avp */
	if (!stream)
		stream=sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpAvp,SalVideo);
	if (!local_stream)
		local_stream = sal_media_description_find_stream(call->localdesc,
		SalProtoRtpAvp,SalVideo);

	if (stream && stream->dir!=SalStreamInactive && stream->port!=0){
		call->video_profile=make_profile(call,call->resultdesc,stream,&used_pt);
		if (used_pt == -1){
			PrintConsole("No video stream accepted ?\n");
		}
		else {
			call->current_params.video_codec = rtp_profile_get_payload(call->video_profile, used_pt);
			PayloadType *p=call->current_params.video_codec;

			if(m_videoModeChoose == 0) //real-time
				startVideoCapture(call);
			else if(m_videoModeChoose == 1) //desktop share
			{
				//TODO:
				startVideoDesktopCapture(call);
			}//desktop share
			else
			{
				PrintConsole(" m_videoModeChoose error !\n");
				return;
			}

			if( videoWindow ) {
#ifdef WIN32
				RECT remoteRect;
				::GetWindowRect((HWND)videoWindow, &remoteRect);
				videoWindowSize = (remoteRect.right-remoteRect.left) * (remoteRect.bottom-remoteRect.top);
#endif
//				ViERender* render =  ViERender::GetInterface(m_vie);
//				render->AddRenderer(call,call->m_VideoChannelID,videoWindow,2,0,0,1,1,/*sean 20130402*/return_video_width_height/*sean*/);
///*				render->AddRenderCallback(call->m_VideoChannelID,base->GetReceiveStatisticsProxy(call->m_VideoChannelID));*/
//				render->StartRender(call->m_VideoChannelID);
//				render->Release();

				ECMedia_add_render(call->m_VideoChannelID,videoWindow,return_video_width_height);
			}

			//network->SetSendDestination(call->m_VideoChannelID,
			//	stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr,stream->port);
			ECMedia_video_set_send_destination(call->m_VideoChannelID,
				stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr, stream->port, stream->rtcp_port);

			cloopenwebrtc::VideoCodec codec_params;			
			bool codec_found = false;
			int num_codec = ECMedia_num_of_supported_codecs_video();
			cloopenwebrtc::VideoCodec *codecArray = new cloopenwebrtc::VideoCodec[num_codec];
			ECMedia_get_supported_codecs_video(codecArray);
			for (int i = 0; i < num_codec; i++) {
				codec_params = codecArray[i];
				if ( strcasecmp( codec_params.plName, p->mime_type) == 0) {
					codec_found = true;
					codec_params.plType = used_pt;
					break;
				}
			}
			delete []codecArray;

			if (!codec_found) {
				PrintConsole("Can not find video codec %s \n", p->mime_type);
			} else {
				if(m_videoBitRates > 0 && m_videoBitRates > codec_params.minBitrate) {
					//codec_params.startBitrate = min(max(m_videoBitRates, kMinVideoBitrate), kMaxVideoBitrate);
					//codec_params.maxBitrate = min(m_videoBitRates*1.5, kMaxVideoBitrate);  //by ylr,20150513
					//codec_params.minBitrate = max(m_videoBitRates/2, kMinVideoBitrate);

					//codec_params.startBitrate = (m_sendVideoWidth*m_sendVideoHeight*m_sendVideoFps*2*0.07)/1000;
					/*codec_params.maxBitrate = min((m_sendVideoWidth*m_sendVideoHeight*m_sendVideoFps*4*0.07)/1000, kMaxVideoBitrate);
					codec_params.minBitrate = max((m_sendVideoWidth*m_sendVideoHeight*m_sendVideoFps*1*0.07)/1000, kMinVideoBitrate);*/

					codec_params.startBitrate = m_sendVideoWidth*m_sendVideoHeight*15*2*0.07/1000;
					codec_params.maxBitrate = 1500;

				}

				codec_params.width = m_sendVideoWidth;
				codec_params.height = m_sendVideoHeight;
				codec_params.maxFramerate = m_sendVideoFps;

				//do some test on simulcast video streams
#ifdef NOTDEFINED
				{
					codec_params.startBitrate = 1300;
					codec_params.maxBitrate = 4000;

					codec_params.numberOfSimulcastStreams = 2;
					codec_params.simulcastStream[0].maxBitrate = codec_params.maxBitrate / 4;;
					codec_params.simulcastStream[0].width = m_sendVideoWidth / 2;
					codec_params.simulcastStream[0].height = m_sendVideoHeight / 2;
					codec_params.simulcastStream[0].numberOfTemporalLayers = 2;
					codec_params.simulcastStream[0].targetBitrate = 300;
					codec_params.simulcastStream[1].maxBitrate = codec_params.maxBitrate;
					codec_params.simulcastStream[1].width = m_sendVideoWidth;
					codec_params.simulcastStream[1].height = m_sendVideoHeight;
					codec_params.simulcastStream[1].numberOfTemporalLayers = 2;
					codec_params.simulcastStream[1].targetBitrate = 1000;
				}
#endif
				

				if (codec_params.width==160 && codec_params.height==120)
				{
					codec_params.maxFramerate = 15;
					codec_params.startBitrate = 40;
					codec_params.maxBitrate = 80;
					codec_params.minBitrate = 30;
				}else if (codec_params.width == 320 && codec_params.height == 240)
				{
					codec_params.maxFramerate = 15;
					codec_params.startBitrate = 100;
					codec_params.maxBitrate = 200;
					codec_params.minBitrate = 50;
				}
				else if (codec_params.width == 640 && codec_params.height == 480)
				{
					codec_params.maxFramerate = 15;
					codec_params.startBitrate = 250;
					codec_params.maxBitrate = 500;
					codec_params.minBitrate = 125;
				}
				else if (codec_params.width == 1280 && codec_params.height == 720)
				{
					codec_params.maxFramerate = 15;
					codec_params.startBitrate = 500;
					codec_params.maxBitrate = 1000;
					codec_params.minBitrate = 250;
				}
				

				if (m_videoModeChoose == 1)
				{
					codec_params.mode = cloopenwebrtc::kScreensharing;
					codec_params.startBitrate = m_desktop_bit_rate;
				}

				PrintConsole("Video Codec is : playload type = %d, payload name is %s  bitrate=%d width=%d height=%d\n",
					codec_params.plType, codec_params.plName, codec_params.startBitrate, codec_params.width, codec_params.height);

				if (ECMedia_set_send_codec_video(call->m_VideoChannelID, codec_params) < 0)
				{
					PrintConsole("Error: ECMedia_set_send_codec_video() fail!");
				}

				if (ECMedia_set_receive_codec_video(call->m_VideoChannelID,codec_params) < 0)
				{
					PrintConsole("Error: ECMedia_set_receive_codec_video() fail!");
				}
#ifdef WIN32 //for ulpfec debug
				memset(codec_params.plName, 0, cloopenwebrtc::kPayloadNameSize);
				memcpy(codec_params.plName, "red", 3);
				codec_params.plType = 116;
				codec_params.codecType = cloopenwebrtc::kVideoCodecRED;
				if (ECMedia_set_receive_codec_video(call->m_VideoChannelID,codec_params) < 0)
				{
					PrintConsole("Error: ECMedia_set_receive_codec_video() fail!");
				}
				memset(codec_params.plName, 0, cloopenwebrtc::kPayloadNameSize);
				memcpy(codec_params.plName, "ulpfec", 6);
				codec_params.plType = 117;
				codec_params.codecType = cloopenwebrtc::kVideoCodecULPFEC;
				if (ECMedia_set_receive_codec_video(call->m_VideoChannelID,codec_params) < 0)
				{
					PrintConsole("Error: ECMedia_set_receive_codec_video() fail!");
				}
#endif
				//pSendStats_ = Serphone_set_video_send_statistics_proxy(call->m_VideoChannelID);
				//pReceiveStats_ = Serphone_set_video_receive_statistics_porxy(call->m_VideoChannelID);

				//TODO:
				////add by ylr 20151010
				//if (!call->vie_observer)
				//{
				//	call->vie_observer = new VieObserver(this);
				//	codec->RegisterDecoderObserver(call->m_VideoChannelID, *call->vie_observer);
				//}	
			}

#ifdef	ENABLE_RECORD_RAW_VIDEO
			{
				//ViEFile *file_record = ViEFile::GetInterface(m_vie);
				//CodecInst audio_codec;
				//memcpy(codec_params.plName, "I420", kPayloadNameSize);
				//file_record->StartRecordIncomingVideo(call->m_VideoChannelID, "incoming_video.avi", AudioSource::NO_AUDIO,audio_codec, codec_params);
				//file_record->StartRecordOutgoingVideo(call->m_VideoChannelID, "outgoing_video.avi", AudioSource::NO_AUDIO, audio_codec, codec_params);
				//file_record->Release();

				//TODO:
 				//ViECodec *vie_codec = ViECodec::GetInterface(m_vie);
				//if (vie_codec)
				//{
				//	vie_codec->StartDebugRecording(call->m_VideoChannelID, "input.yuv");
				//	vie_codec->Release();
				//}
			}		
#endif
			{
				PrintConsole("ENABLE_REMB_TMMBR_CONFIG: videoNackEnabled=%d  true, \n", videoNackEnabled);
				ECMedia_set_NACK_status_video(call->m_VideoChannelID, videoNackEnabled);
				ECMedia_set_RTCP_status_video(call->m_VideoChannelID, 2/*kRtcpNonCompound_RFC5506*/);
#ifdef WIN32
				switch(videoProtectionMode)
				{
				case 0:
					ECMedia_set_NACK_status_video(call->m_VideoChannelID, videoNackEnabled);
					break;
				case 1:
					ECMedia_set_NACK_status_video(call->m_VideoChannelID, false);
					ECMedia_set_FEC_status_video(call->m_VideoChannelID, TRUE, 116/*RED*/, 117/*FEC*/);
					break;
				case 2:
					ECMedia_set_HybridNACKFEC_status_video(call->m_VideoChannelID, TRUE, 116/*RED*/, 117/*FEC*/);
					break;
				default:
					break;
				}
#endif
			}
			PrintConsole("ENABLE_REMB_TMMBR_CONFIG: videoNackEnabled=%d  false, \n", videoNackEnabled);

#ifdef ENABLE_REMB_TMMBR_CONFIG
			//TODO:
			//PrintConsole("ENABLE_REMB_TMMBR_CONFIG: rembEnabled=%d, tmmbrEnabled=%d\n", rembEnabled, tmmbrEnabled);
			//rtp_rtcp->SetRembStatus(call->m_VideoChannelID, rembEnabled, rembEnabled);
			//rtp_rtcp->SetTMMBRStatus(call->m_VideoChannelID, tmmbrEnabled);
			//if (rembEnabled)
			//{
			//	rtp_rtcp->SetSendAbsoluteSendTimeStatus(call->m_VideoChannelID, true, kRtpExtensionAbsoluteSendTime);
			//	//rtp_rtcp->SetSendTimestampOffsetStatus(call->m_VideoChannelID, true, kRtpExtensionTransmissionTimeOffset);

			//	rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(call->m_VideoChannelID, true, kRtpExtensionAbsoluteSendTime);
			//	//rtp_rtcp->SetReceiveTimestampOffsetStatus(call->m_VideoChannelID, true, kRtpExtensionTransmissionTimeOffset);

			//}
#endif
			if ( local_stream){
				switch(local_stream->dir)
				{
				case SalStreamSendOnly:
					ECMedia_video_start_send(call->m_VideoChannelID);
					break;
				case SalStreamRecvOnly:
					ECMedia_video_start_receive(call->m_VideoChannelID);
					break;
				case SalStreamInactive:
					break;
				default:
					ECMedia_video_start_send(call->m_VideoChannelID);
					ECMedia_video_start_receive(call->m_VideoChannelID);
					break;
				}
			}
			else{
				ECMedia_video_start_send(call->m_VideoChannelID);
				ECMedia_video_start_receive(call->m_VideoChannelID);
			}

#ifdef _WIN32         
			//TODO:
			//if(call->record_voip && call->record_voip->isStartRecordMp4()) {
			//	ViEFile *file = ViEFile::GetInterface(m_vie);
			//	if(file) {
			//		file->RegisterVideoFrameStorageCallBack(call->m_VideoChannelID, call->record_voip);
			//		rtp_rtcp->RequestKeyFrame(call->m_VideoChannelID);
			//	}
			//	file->Release();
			//}
#endif
			call->current_params.in_conference=call->params.in_conference;
		}
	}

#endif
}


int ServiceCore::startVideoCapture(SerPhoneCall *call) 
{
#ifdef VIDEO_ENABLED
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);

	if(!call) {
		PrintConsole("startVideoCapture failed. call=%0x\n", call);
		return -3;
	}

	if(call->m_CaptureDeviceId != -1) {
		PrintConsole("startVideoCapture failed. already captured call->m_CaptureDeviceId=%d\n",call->m_CaptureDeviceId);
		return -4;
	}

	if(!call->params.has_video) {
		PrintConsole("startVideoCapture failed. this call is not video call\n");
		return -5;
	}

	//ViEBase* base = ViEBase::GetInterface(m_vie);
	//ViECapture *capture = ViECapture::GetInterface(m_vie);

	if(!m_cameraInfo) {
		CameraInfo *info;
		getCameraInfo(&info);
	}

	char name[256], id[256];
	if ( ECMedia_get_capture_device(m_usedCameraIndex,name,sizeof(name),id,sizeof(id)) < 0)  {
		PrintConsole("Can not find video device \n");
		if(vtable.connect_camera_failed)
			vtable.connect_camera_failed(this, this->current_call, m_usedCameraIndex, "");
	} else {	
		{
			CameraCapability &cc = m_cameraInfo[m_usedCameraIndex].capability[m_usedCapabilityIndex];
			CameraCapability cap;
			cap.height = cc.height;
			cap.width = cc.width;
			cap.maxfps = m_maxFPS;
			ECMedia_allocate_capture_device(id,strlen(id),call->m_CaptureDeviceId);
            ECmedia_enable_deflickering(call->m_CaptureDeviceId, true);
            ECmedia_enable_EnableBrightnessAlarm(call->m_CaptureDeviceId, true);
//            ECmedia_enable_EnableDenoising(call->m_CaptureDeviceId, true);
			//TODO:
			//capture->SetSendStatisticsProxy(call->m_CaptureDeviceId, base->GetSendStatisticsProxy(call->m_VideoChannelID));
			ECMedia_set_CaptureDeviceID(call->m_CaptureDeviceId);
		}		

		if( ECMedia_connect_capture_device(call->m_CaptureDeviceId,call->m_VideoChannelID) < 0 ) {
			PrintConsole("Open Camera:%s Failed!  \n", name);
			if(vtable.connect_camera_failed)
				vtable.connect_camera_failed(this, this->current_call, m_usedCameraIndex, name);
		}

		if( !m_cameraInfo 
			|| m_usedCameraIndex >= m_cameraCount
			|| !m_cameraInfo[m_usedCameraIndex].capability 
			|| m_usedCapabilityIndex >= m_cameraInfo[m_usedCameraIndex].capabilityCount ) {
				PrintConsole("Invalid CameraIndex(%d) or capabilityIndex(%d)\n", m_usedCameraIndex, m_usedCapabilityIndex);
				if(vtable.connect_camera_failed)
					vtable.connect_camera_failed(this, this->current_call, m_usedCameraIndex, "");

		} else  {              
			CameraCapability &cc = m_cameraInfo[m_usedCameraIndex].capability[m_usedCapabilityIndex];
			CameraCapability cap;

			cap.height = cc.height;
			cap.width = cc.width;
			cap.maxfps = m_maxFPS;

			//capability_conf.hdvideo = 0; //for test, by ylr 
//			if(capability_conf.hdvideo) {
#ifdef WEBRTC_ANDROID
				m_sendVideoWidth = cap.height;
				m_sendVideoHeight = cap.width;
#else
				m_sendVideoWidth = cap.width;
				m_sendVideoHeight = cap.height;
#endif
				m_sendVideoFps = cap.maxfps;
//			}

			ECMediaRotateCapturedFrame tr = (ECMediaRotateCapturedFrame)m_camerRotate;
			if(m_camerRotate == -1)  {
				ECMedia_getOrientation(id,tr);
				PrintConsole("GetOrientation %d \n", tr);
			}

			//TODO:
			//capture->RegisterObserver(call->m_CaptureDeviceId, *this);
			ECMedia_set_rotate_captured_frames(call->m_CaptureDeviceId,tr);
			ECMedia_start_capture(call->m_CaptureDeviceId,cap);

			PrintConsole("Use No %d camera:%s,height:%d,width:%d,framerate:%d,roate=%d \n",
				m_usedCameraIndex,name,cap.height,cap.width,cap.maxfps,tr);

			if( localVideoWindow ) {
				ECMedia_set_local_video_window(call->m_CaptureDeviceId,localVideoWindow);
			}
		}
	}
	return 0;

#endif
	return -1;
}

int ServiceCore::startVideoDesktopCapture(SerPhoneCall *call)
{
#ifdef VIDEO_ENABLED
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);

	if (!call) {
		PrintConsole("startVideoCapture failed. call=%0x\n", call);
		return -3;
	}

	if (call->m_desktopShareDeviceId != -1) {
		PrintConsole("startVideoCapture failed. already captured call->m_CaptureDeviceId=%d\n", call->m_CaptureDeviceId);
		return -4;
	}

	if (!call->params.has_video) {
		PrintConsole("startVideoCapture failed. this call is not video call\n");
		return -5;
	}
	int type = 1; // ShareScreen=0 ShareWindow=1
	ScreenID *screenId;
	WindowShare *windowInfo;
	{
		ECMedia_allocate_desktopShare_capture(call->m_desktopShareDeviceId, type);
		ECMedia_set_CaptureDeviceID(call->m_desktopShareDeviceId);
		if (type==0)
		{
			getShareScreenInfo(&screenId, call->m_desktopShareDeviceId);
		}else if (type == 1)
		{
			getShareWindowInfo(&windowInfo, call->m_desktopShareDeviceId);
		}
		
	}
	if (call->m_desktopShareDeviceId > 0)
	{
		int  ret;
		if (type==0 && m_pScreenInfo)
		{
			if (ECMedia_select_screen(call->m_desktopShareDeviceId, m_pScreenInfo[0]))
			{
				ECMedia_stop_desktop_capture(call->m_desktopShareDeviceId);
				ECMedia_connect_desktop_captureDevice(call->m_desktopShareDeviceId, call->m_VideoChannelID);
				ECMedia_start_desktop_capture(call->m_desktopShareDeviceId, 15);
				if (localVideoWindow) {
					ECMedia_set_local_video_window(call->m_desktopShareDeviceId, localVideoWindow);
				}
			}
		}else if (type == 1 && m_pWindowInfo)
		{
			if (ECMedia_select_window(call->m_desktopShareDeviceId, m_pWindowInfo[0].id))
			{
				ECMedia_stop_desktop_capture(call->m_desktopShareDeviceId);
				ECMedia_connect_desktop_captureDevice(call->m_desktopShareDeviceId, call->m_VideoChannelID);
				ECMedia_start_desktop_capture(call->m_desktopShareDeviceId, 15);
				if (localVideoWindow) {
					ECMedia_set_local_video_window(call->m_desktopShareDeviceId, localVideoWindow);
				}
			}
		}
	}
#else
#endif
}
int ServiceCore::selectCamera(int cameraIndex, int capabilityIndex,int fps,int rotate, bool force)
{
#ifdef VIDEO_ENABLED
	if (!m_cameraInfo)  {
		PrintConsole("selectCamera m_vie or m_cameraInfo is NULL\n");
		return -1;
	}
	if( cameraIndex < 0 || cameraIndex >= m_cameraCount) {
		PrintConsole("selectCamera cameraIndex is overflowing. cameraIndex=%d m_cameraCount=%d\n",cameraIndex, m_cameraCount);
		return -1;
	}
	if( capabilityIndex < 0 || capabilityIndex >= m_cameraInfo[cameraIndex].capabilityCount ) {
		PrintConsole("selectCamera capabilityIndex is overflowing. capabilityIndex=%d capabilityCount=%d\n", capabilityIndex, m_cameraInfo[cameraIndex].capabilityCount);
		return -1;
	}

	m_maxFPS = fps;
	m_sendVideoFps = fps; //m_semdVideoFps: define as encoder's parameter. add by ylr, 20150513

	switch( rotate ) {
	case ROTATE_AUTO:
		m_camerRotate = -1;
		break;
	case ROTATE_0:
		m_camerRotate = ECMediaRotateCapturedFrame_0;
		break;
	case ROTATE_90:
		m_camerRotate = ECMediaRotateCapturedFrame_90;
		break;
	case ROTATE_180:
		m_camerRotate = ECMediaRotateCapturedFrame_180;
		break;
	case ROTATE_270:
		m_camerRotate = ECMediaRotateCapturedFrame_270;
		break;
	default:
		m_camerRotate = ECMediaRotateCapturedFrame_0;
		break;
	}

	SerPhoneCall *call = serphone_core_get_current_call();
	if( call!= NULL && call->m_CaptureDeviceId != -1 ) {
		if(!force && cameraIndex == m_usedCameraIndex && capabilityIndex == m_usedCapabilityIndex) {
			ECMedia_set_rotate_captured_frames(call->m_CaptureDeviceId,(ECMediaRotateCapturedFrame)m_camerRotate);
		} else {
			char name[256], id[256];
			m_usedCameraIndex = cameraIndex;
			m_usedCapabilityIndex = capabilityIndex;
			if( call->m_CaptureDeviceId >= 0)  {
				ECMedia_stop_capture(call->m_CaptureDeviceId);
			};
			if ( ECMedia_get_capture_device(m_usedCameraIndex,name,sizeof(name),id,sizeof(id)) < 0)  {
				PrintConsole("Can not find video device \n");
				if(vtable.connect_camera_failed)
					vtable.connect_camera_failed(this, this->current_call, m_usedCameraIndex, "");
			} 
			else {
				ECMedia_allocate_capture_device(id,strlen(id),call->m_CaptureDeviceId);
                ECmedia_enable_deflickering(call->m_CaptureDeviceId, true);
                ECmedia_enable_EnableBrightnessAlarm(call->m_CaptureDeviceId, true);
//                ECmedia_enable_EnableDenoising(call->m_CaptureDeviceId, true);
				ECMedia_set_CaptureDeviceID(call->m_CaptureDeviceId);
				if( ECMedia_connect_capture_device(call->m_CaptureDeviceId,call->m_VideoChannelID) < 0 ) {
					PrintConsole("Open Camera:%s Failed!  \n", name);
					if(vtable.connect_camera_failed)
						vtable.connect_camera_failed(this, this->current_call, m_usedCameraIndex, name);
				}

				CameraCapability &cc = m_cameraInfo[m_usedCameraIndex].capability[m_usedCapabilityIndex];
				CameraCapability cap;
				cap.height = cc.height;
				cap.width = cc.width;				
				cap.maxfps = m_maxFPS; 

//				if(capability_conf.hdvideo) {
#ifdef WEBRTC_ANDROID
					m_sendVideoWidth = cap.height;
					m_sendVideoHeight = cap.width;
#else
					m_sendVideoWidth = cap.width;
					m_sendVideoHeight = cap.height;
#endif
					m_sendVideoFps = cap.maxfps;
//				}

				ECMediaRotateCapturedFrame tr = (ECMediaRotateCapturedFrame)m_camerRotate;
				if(m_camerRotate == -1)  {
					ECMedia_getOrientation(id,tr);
				}
				ECMedia_set_rotate_captured_frames(call->m_CaptureDeviceId,tr);
				ECMedia_start_capture(call->m_CaptureDeviceId,cap);

				if( localVideoWindow ) {
					ECMedia_set_local_video_window(call->m_CaptureDeviceId, localVideoWindow);
				}
				PrintConsole("Use No %d camera:%s,height:%d,width:%d,framerate:%d,rotate=%d \n",
					m_usedCameraIndex,name,cap.height,cap.width,cap.maxfps,tr);

				cloopenwebrtc::VideoCodec tempVideoCodec;
				ECMedia_get_send_codec_video(call->m_VideoChannelID, tempVideoCodec);
				tempVideoCodec.width = m_sendVideoWidth;
				tempVideoCodec.height = m_sendVideoHeight;
				tempVideoCodec.maxFramerate = m_sendVideoFps;
				ECMedia_set_send_codec_video(call->m_VideoChannelID, tempVideoCodec);
			}
		}
	}

	m_usedCameraIndex = cameraIndex;
	m_usedCapabilityIndex = capabilityIndex;
	return 0;
#else
	return 0;
#endif
}

int ServiceCore::getCameraInfo(CameraInfo **info)
{
#ifdef VIDEO_ENABLED
	if(m_cameraInfo)
	{
		for(int i=0; i< m_cameraCount;i++){
			delete[] m_cameraInfo[i].capability;
			m_cameraInfo[i].capability = NULL;
		}
		delete[] m_cameraInfo;
		m_cameraInfo = NULL;
	}

	//ViECapture* capture = ViECapture::GetInterface(m_vie);
	if(!m_cameraInfo) {
		m_cameraCount = ECMdeia_num_of_capture_devices();
		if( m_cameraCount > 0) {
			m_cameraInfo = new CameraInfo[m_cameraCount];
			for( int i=0; i< m_cameraCount ; i++ ) {
				char name[256], id[256];
				memset(&m_cameraInfo[i], 0, sizeof(CameraInfo));
				if ( ECMedia_get_capture_device(i,name,sizeof(name),id,sizeof(id)) == 0)  {
					m_cameraInfo[i].index = i;
					strcpy(m_cameraInfo[i].name,name);
					strcpy(m_cameraInfo[i].id, id);
					PrintConsole(" camara device[%d] name[%s] id[%s]\n",i,name,id);

					m_cameraInfo[i].capabilityCount = ECMedia_num_of_capabilities(id,sizeof(id));
					if( m_cameraInfo[i].capabilityCount >0 )
					{
						m_cameraInfo[i].capability = new CameraCapability[m_cameraInfo[i].capabilityCount];
						memset(m_cameraInfo[i].capability, 0, sizeof(CameraCapability));
						strcpy(m_cameraInfo[i].name,name);
						for( int j =0; j<m_cameraInfo[i].capabilityCount ; j++ ) {
							CameraCapability capability;
							ECMedia_get_capture_capability(id,sizeof(id),j,capability);
							m_cameraInfo[i].capability[j].height =capability.height;
							m_cameraInfo[i].capability[j].width =capability.width;
							m_cameraInfo[i].capability[j].maxfps =capability.maxfps;
							PrintConsole("camera[%d] capability[%d] height:%d ,width:%d,maxFPS:%d\n",
								i,j,capability.height, capability.width,capability.maxfps);
						}
					}					
				}
			}
		}
	}
	*info =  m_cameraInfo;
	return m_cameraCount;
#else
	return 0;
#endif
}

int ServiceCore::getCallStatistics(int type,MediaStatisticsInfo *callStats)
{
//	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
#if !defined(NO_VOIP_FUNCTION)
	if( type == 0)
	{
		SerPhoneCall *call =serphone_core_get_current_call();
		if( call == NULL ||call->m_AudioChannelID == -1 ) {
			memset(callStats,0,sizeof(MediaStatisticsInfo));
			return -1;
		}

		ECMedia_get_media_statistics(call->m_AudioChannelID, false, *callStats);
	}
#ifdef VIDEO_ENABLED
	else if( type == 1 )
	{
		SerPhoneCall *call =serphone_core_get_current_call();
		if( call == NULL ||call->m_VideoChannelID == -1) {
			memset(callStats,0,sizeof(MediaStatisticsInfo));
			return -1;
		}
		ECMedia_get_media_statistics(call->m_VideoChannelID, true, *callStats);
	}
#endif
#endif
	return 0;
}

int ServiceCore::serphone_set_louds_speaker_status(bool bLouds)
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
#if !defined(NO_VOIP_FUNCTION)
	return ECMedia_set_loudspeaker_status(bLouds);
#endif
	return 0;
}

int ServiceCore::serphone_get_louds_speaker_status()
{
    cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
#if !defined(NO_VOIP_FUNCTION)
	bool bLouds = false;
	ECMedia_get_loudpeaker_status(bLouds);
	return (bLouds ? 1 : 0);
#else
	return 0;
#endif
}

int ServiceCore::serphone_set_mute_status(bool muted)
{
#if !defined(NO_VOIP_FUNCTION)
	return ECMedia_set_mute_status(muted);
#endif
	return -1;
}
int ServiceCore::serphone_get_mute_status()
{
#if !defined(NO_VOIP_FUNCTION)
	bool enable;
	ECMedia_get_mute_status(enable);
	return (enable ? 1 : 0 );
#else
	return 0;
#endif
}

int ServiceCore::serphone_set_speaker_mute_status(bool muted)
{
#if !defined(NO_VOIP_FUNCTION)
	return ECMedia_set_speaker_mute_status(muted);
#endif
	return -1;
}

int ServiceCore::serphone_get_speaker_mute_status()
{
#if !defined(NO_VOIP_FUNCTION)
	bool enable;
	ECMedia_get_speaker_mute_status(enable);
	return (enable ? 1 : 0 );
#else
	return 0;
#endif
}

int ServiceCore::serphone_core_get_audio_dscp(const ServiceCore *lc){
	return lp_config_get_int(lc->config,"rtp","audio_dscp",0x2e);
}

const char * ServiceCore::media_stream_type_str(MediaStream *stream) {
	switch (stream->type) {
	default:
	case AudioStreamType:
		return "audio";
	case VideoStreamType:
		return "video";
	}
}

int ServiceCore::media_stream_set_dscp(MediaStream *stream, int dscp) {
	PrintConsole("Setting DSCP to %i for %s stream.\n", dscp, media_stream_type_str(stream));
	return rtp_session_set_dscp(stream->session, dscp);
}

int ServiceCore::audio_stream_set_dscp(AudioStream *stream, int dscp) {
	return media_stream_set_dscp(&stream->ms, dscp);
}

bool_t ServiceCore::serphone_core_echo_limiter_enabled(const ServiceCore *lc){
	return lc->sound_conf.ea;
}

void ServiceCore::serphone_call_init_audio_stream(SerPhoneCall *call){
	AudioStream *audiostream;

	if (call->audiostream != NULL)
	{
		return;
	}
	call->audiostream=audiostream=audio_stream_new(call->audio_port,call->audio_port+1,serphone_core_ipv6_enabled(),call->m_AudioChannelID);
	if ((serphone_core_get_firewall_policy() == LinphonePolicyUseIce) && (call->ice_session != NULL)){
		rtp_session_set_pktinfo(audiostream->ms.session, TRUE);
		rtp_session_set_symmetric_rtp(audiostream->ms.session, FALSE);
		if (ice_session_check_list(call->ice_session, 0) == NULL) {
			ice_session_add_check_list(call->ice_session, ice_check_list_new());
		}
		audiostream->ms.ice_check_list = ice_session_check_list(call->ice_session, 0);
		ice_check_list_set_rtp_session(audiostream->ms.ice_check_list, audiostream->ms.session);
	}
	call->audiostream_app_evq = ortp_ev_queue_new();
	rtp_session_register_event_queue(audiostream->ms.session,call->audiostream_app_evq);
}

void ServiceCore::serphone_call_init_video_stream(SerPhoneCall *call)
{ 
#ifdef VIDEO_ENABLED
	ServiceCore *lc=call->core;

	if (!call->params.has_video) {//sean todo problem
#ifdef VIDEO_ENABLED
		if (call->videostream!=NULL){
			rtp_session_unregister_event_queue(call->videostream->ms.session,call->videostream_app_evq);
			ortp_ev_queue_flush(call->videostream_app_evq);
			ortp_ev_queue_destroy(call->videostream_app_evq);
			call->videostream_app_evq=NULL;
			video_stream_stop(call->m_VideoChannelID, call->m_CaptureDeviceId);
			call->videostream=NULL;
		}
#endif
		return;
	}
	if (call->videostream != NULL) return;
	//    if ((lc->video_conf.display || lc->video_conf.capture) && call->params.has_video){
	if (call->params.has_video){		
		call->videostream=video_stream_new(call->video_port,call->video_port+1,serphone_core_ipv6_enabled(),call->m_VideoChannelID);
		if ((serphone_core_get_firewall_policy() == LinphonePolicyUseIce) && (call->ice_session != NULL)){
			rtp_session_set_pktinfo(call->videostream->ms.session, TRUE);
			rtp_session_set_symmetric_rtp(call->videostream->ms.session, FALSE);
			if (ice_session_check_list(call->ice_session, 1) == NULL) {
				ice_session_add_check_list(call->ice_session, ice_check_list_new());
			}
			call->videostream->ms.ice_check_list = ice_session_check_list(call->ice_session, 1);

			ice_check_list_set_rtp_session(call->videostream->ms.ice_check_list, call->videostream->ms.session);
		}
		call->videostream_app_evq = ortp_ev_queue_new();
		rtp_session_register_event_queue(call->videostream->ms.session,call->videostream_app_evq);
#ifdef TEST_EXT_RENDERER
		video_stream_set_render_callback(call->videostream,rendercb,NULL);
#endif
	}
#else
	call->videostream=NULL;
#endif
}


void ServiceCore::serphone_call_init_media_streams(SerPhoneCall *call)
{
#if !defined(NO_VOIP_FUNCTION)
	SalMediaDescription *md=call->localdesc;
#ifndef WIN32
	ECMedia_init_audio();
#endif
	ECMedia_audio_create_channel(call->m_AudioChannelID, false);

    //TODO:
	//base->SetFecStatus(call->m_AudioChannelID, m_enable_fec);
    //base->SetLoss(call->m_AudioChannelID, m_opus_packet_loss_rate);
	ECMedia_set_network_type(call->m_AudioChannelID, -1, networkType);
	serphone_core_set_process_audio_data_flag_internel(call);
	ECMedia_set_local_receiver(call->m_AudioChannelID, md->streams[0].port, md->streams[0].port+1);

	//audio->EnableHighPassFilter(true);
	ECMedia_set_AgcStatus(m_agcEnabled, m_agcMode);
    //for MOS test
//            audio->SetAecmMode(kAecmLoudSpeakerphone,true);
//			audio->SetEcStatus(m_ecEnabled, m_ecMode);
//			//audio->SetNsStatus(m_nsEnabled, m_nsMode);
//			audio->SetNsStatus(m_nsEnabled, cloopenwebrtc::kNsVeryHighSuppression);
            
    ECMedia_set_AgcStatus(false, m_agcMode);
    ECMedia_set_EcStatus(true, m_ecMode);
    ECMedia_set_NsStatus(true, cloopenwebrtc::kNsVeryHighSuppression);
    ECMedia_EnableHowlingControl(m_hcEnabled);

	//Init Srtp
	//sean20130428
	if (srtp_enable) {
		int err = ECMedia_init_srtp(call->m_AudioChannelID);
		if (err) {
			PrintConsole("Init SRTP fail code [%d]\n",err);
		}
	}
		
	if (serphone_core_get_firewall_policy() == LinphonePolicyUseIce && NULL != call->ice_session)
	{
		serphone_call_init_audio_stream(call);
		//TODO:
		//call->audiostream->ms.session->VoEBase = base;
		ECMedia_audio_start_receive(call->m_AudioChannelID);
	}
#ifdef VIDEO_ENABLED

	if (call->params.has_video) {
		ECMedia_audio_create_channel(call->m_VideoChannelID, true);
//        sean for multivideo encoding begin
//        ECMedia_audio_create_channel(call->m_VideoChannelID1, true);
//        ECMedia_audio_create_channel(call->m_VideoChannelID2, true);
//        sean for multivideo encoding end
        

		if( call->m_VideoChannelID >= 0 &&  md->nstreams > 1 ) {
			ECMedia_set_network_type(call->m_AudioChannelID, call->m_VideoChannelID, networkType);
			ECMedia_video_set_local_receiver(call->m_VideoChannelID,call->video_port, call->video_port+1);
			ECMedia_set_MTU(call->m_VideoChannelID,1450);

			/*add begin------------------Sean20130722----------for video ice------------*/
			//            sean update begin 0926 ice bug when no stunserver
			//            if (serphone_core_get_firewall_policy() == LinphonePolicyUseIce) {
			//TODO:
			//network->RegisterServiceCoreCallBack(call->m_VideoChannelID, this, call->_user_call_id, serphone_core_get_firewall_policy());

			if (serphone_core_get_firewall_policy() == LinphonePolicyUseIce && NULL != call->ice_session) {
				//            sean update end 0926 ice bug when no stunserver
				serphone_call_init_video_stream(call);
				//set send socket
				//TODO:
				//call->videostream->ms.session->ViENetwork = network;
				ECMedia_video_start_receive(call->m_VideoChannelID);
			}
			/*add end--------------------Sean20130722----------for video ice------------*/
		}
	}
#endif
#endif
}

/**
* @ingroup IOS
* Special function to warm up  dtmf feeback stream. #linphone_core_stop_dtmf_stream must() be called before entering FG mode
*/
void ServiceCore::serphone_core_start_dtmf_stream()
{
}

/**
* @ingroup IOS
* Special function to stop dtmf feed back function. Must be called before entering BG mode
*/
void ServiceCore::serphone_core_stop_dtmf_stream()
{
}

void ServiceCore::serphone_core_stop_dtmf( )
{
}

void ServiceCore::serphone_call_enable_camera(SerPhoneCall *lc, bool_t enabled)
{
}
bool_t ServiceCore::serphone_call_camera_enabled(const SerPhoneCall *lc)
{
	return TRUE;
}

void ServiceCore::serphone_core_mute_mic(bool_t muted)
{
}

bool_t ServiceCore::serphone_core_is_mic_muted()
{
	return TRUE;
}


void ServiceCore::audio_stream_prepare_sound(SerPhoneCall *call)
{
	//	audio_stream_unprepare_sound(stream);
	//	stream->dummy=ms_filter_new(MS_RTP_RECV_ID);
	//	rtp_session_set_payload_type(stream->ms.session,0);
	//	ms_filter_call_method(stream->dummy,MS_RTP_RECV_SET_SESSION,stream->ms.session);
	//    
	//	if (captcard && playcard){
	//#ifdef __ios
	//		stream->soundread=ms_snd_card_create_reader(captcard);
	//		stream->soundwrite=ms_snd_card_create_writer(playcard);
	//		ms_filter_link(stream->dummy,0,stream->soundwrite,0);
	//#else
	//		stream->ms.voidsink=ms_filter_new(MS_VOID_SINK_ID);
	//		ms_filter_link(stream->dummy,0,stream->ms.voidsink,0);
	//#endif
	//	} else {
	//		stream->ms.voidsink=ms_filter_new(MS_VOID_SINK_ID);
	//		ms_filter_link(stream->dummy,0,stream->ms.voidsink,0);
	//	}
	//	if (stream->ms.ticker == NULL) start_ticker(&stream->ms);
	//	ms_ticker_attach(stream->ms.ticker,stream->dummy);
}

void ServiceCore::audio_stream_unprepare_sound(SerPhoneCall *call)
{
#ifdef __ios
	... ...
#endif
}

void ServiceCore::video_stream_prepare_video(VideoStream *stream)
{
	//	stream->prepare_ongoing = TRUE;
	//	video_stream_unprepare_video(stream);
	//	stream->ms.rtprecv=ms_filter_new(MS_RTP_RECV_ID);
	//	rtp_session_set_payload_type(stream->ms.session,0);
	//	ms_filter_call_method(stream->ms.rtprecv,MS_RTP_RECV_SET_SESSION,stream->ms.session);
	//	stream->ms.voidsink=ms_filter_new(MS_VOID_SINK_ID);
	//	ms_filter_link(stream->ms.rtprecv,0,stream->ms.voidsink,0);
	//	start_ticker(&stream->ms);
	//	ms_ticker_attach(stream->ms.ticker,stream->ms.rtprecv);
}

void ServiceCore::video_stream_unprepare_video(VideoStream *stream)
{
}

void ServiceCore::audio_stream_play(SerPhoneCall *call, const char *name)
{
}

void ServiceCore::serphone_core_send_dtmf(char dtmfch)
{
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);

    ////hubintest
    //if(dtmfch == '1') {
    //    ECMedia_EnableHowlingControl(true);
    //}
    //else if(dtmfch == '2') {
    //    ECMedia_EnableHowlingControl(false);
    //}    
    
#if !defined(NO_VOIP_FUNCTION)
	SerPhoneCall *call =serphone_core_get_current_call();
	ECMedia_send_dtmf(call->m_AudioChannelID, dtmfch);
#endif
}

void ServiceCore::serphone_core_playfile_to_remote(SerPhoneCall *call,char * filename)
{
#if !defined(NO_VOIP_FUNCTION)
	if (call==NULL){
		PrintConsole("serphone_core_playfile_to_remote: call null,exit\n");
		return;
	}
	//TODO:
	//ring_stop();
	//if (call->m_AudioChannelID >= 0 && m_voe ){
	//	VoEBase* base = VoEBase::GetInterface(m_voe);
	//	VoEFile* file  = VoEFile::GetInterface(m_voe);
	//	if ( file->IsPlayingFileAsMicrophone(call->m_AudioChannelID) >=0)
	//		file->StopPlayingFileAsMicrophone( call->m_AudioChannelID );
	//	file->StartPlayingFileAsMicrophone( call->m_AudioChannelID,filename,true);
	//	base->StartPlayout(call->m_AudioChannelID);
	//	file->Release();
	//	base->Release();
	//}
#endif
}

void ServiceCore::serphone_core_stop_playfile_to_remote(SerPhoneCall *call)
{
#if !defined(NO_VOIP_FUNCTION)
	if (call==NULL){
		PrintConsole("serphone_core_playfile_to_remote: call null,exit\n");
		return;
	}
	//TODO:
	//if (call->m_AudioChannelID >= 0 && m_voe ){
	//	VoEBase* base = VoEBase::GetInterface(m_voe);
	//	VoEFile* file  = VoEFile::GetInterface(m_voe);
	//	if ( file->IsPlayingFileAsMicrophone(call->m_AudioChannelID) >=0)
	//		file->StopPlayingFileAsMicrophone( call->m_AudioChannelID );

	//	file->Release();
	//	base->Release();
	//}
#endif
}

void serphone_core_set_android_objects(void* javaVM, void* env, void* context)
{
	ECMedia_set_android_objects(javaVM, env, context);
}

int ServiceCore::serphone_core_set_audio_config_enabled(int type, bool_t enabled, int mode)
{
#if !defined(NO_VOIP_FUNCTION)
	PrintConsole("serphone_core_set_audio_config_enabled type=%d enabled=%d mode=%d\n", type, enabled, mode);
	switch (type) {
	case AUDIO_AGC:
		m_agcEnabled = enabled;
		if(mode != cloopenwebrtc::kAgcUnchanged)
			m_agcMode = (cloopenwebrtc::AgcModes)mode;
		break;
	case AUDIO_EC:
		m_ecEnabled = enabled;
		if(mode != cloopenwebrtc::kEcUnchanged)
			m_ecMode = (cloopenwebrtc::EcModes)mode;
		break;
	case AUDIO_NS:
		m_nsEnabled = enabled;
		if(mode != cloopenwebrtc::kNsUnchanged)
			m_nsMode = (cloopenwebrtc::NsModes)mode;
		break;
    case AUDIO_HC:
        m_hcEnabled = enabled;
	default:
		return -1;
	}
	ECMedia_set_AgcStatus(m_agcEnabled, m_agcMode);
	ECMedia_set_EcStatus(m_ecEnabled, m_ecMode);
	ECMedia_set_SetAecmMode(cloopenwebrtc::kAecmLoudSpeakerphone, false);
	ECMedia_set_NsStatus(m_nsEnabled, cloopenwebrtc::kNsVeryHighSuppression);
    ECMedia_EnableHowlingControl(m_hcEnabled);
    return 0;
#endif
	return 0;
}

int ServiceCore::serphone_core_get_audio_config_enabled(int type, bool_t *enabled, int *mode)
{
#if !defined(NO_VOIP_FUNCTION)
	bool configEnabled = false;
	int configMode = 0;
	switch (type) {
	case AUDIO_AGC:
		*enabled = m_agcEnabled;
		*mode = m_agcMode;
		break;
	case AUDIO_EC:
		*enabled = m_ecEnabled;
		*mode = m_ecMode;
		break;
	case AUDIO_NS:
		*enabled = m_nsEnabled;
		*mode = m_nsMode;
		break;            
	default:
		return -1;
	}
	PrintConsole("serphone_core_get_audio_config_enabled type=%d enabled=%d mode=%d\n", type, *enabled, *mode);
#endif
	return 0;
}

//sean ice todo
void ServiceCore::serphone_core_update_ice_from_remote_media_description(SerPhoneCall *call, const SalMediaDescription *md)
{
	bool_t ice_restarted = FALSE;

	if ((md->ice_pwd[0] != '\0') && (md->ice_ufrag[0] != '\0')) {
		int i, j;

		/* Check for ICE restart and set remote credentials. */
		if ((strcmp(md->addr, "0.0.0.0") == 0) || (strcmp(md->addr, "::0") == 0)) {
			ice_session_restart(call->ice_session);
			ice_restarted = TRUE;
		} else {
			for (i = 0; i < md->n_total_streams; i++) {
				const SalStreamDescription *stream = &md->streams[i];
				IceCheckList *cl = ice_session_check_list(call->ice_session, i);
				if (cl && (strcmp(stream->rtp_addr, "0.0.0.0") == 0)) {
					//                if (cl && (strcmp(stream->addr, "0.0.0.0") == 0)) {
					ice_session_restart(call->ice_session);
					ice_restarted = TRUE;
					break;
				}
			}
		}
		if ((ice_session_remote_ufrag(call->ice_session) == NULL) && (ice_session_remote_pwd(call->ice_session) == NULL)) {
			ice_session_set_remote_credentials(call->ice_session, md->ice_ufrag, md->ice_pwd);
		} else if (ice_session_remote_credentials_changed(call->ice_session, md->ice_ufrag, md->ice_pwd)) {
			if (ice_restarted == FALSE) {
				ice_session_restart(call->ice_session);
				ice_restarted = TRUE;
			}
			ice_session_set_remote_credentials(call->ice_session, md->ice_ufrag, md->ice_pwd);
		}
		for (i = 0; i < md->n_total_streams; i++) {
			const SalStreamDescription *stream = &md->streams[i];
			IceCheckList *cl = ice_session_check_list(call->ice_session, i);

			if (cl && (stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0')) {
				if (ice_check_list_remote_credentials_changed(cl, stream->ice_ufrag, stream->ice_pwd)) {
					if (ice_restarted == FALSE) {
						ice_session_restart(call->ice_session);
						ice_restarted = TRUE;
					}
					ice_session_set_remote_credentials(call->ice_session, md->ice_ufrag, md->ice_pwd);
					break;
				}
			}
		}

		/* Create ICE check lists if needed and parse ICE attributes. */
		for (i = 0; i < md->n_total_streams; i++) {
			const SalStreamDescription *stream = &md->streams[i];
			IceCheckList *cl = ice_session_check_list(call->ice_session, i);
			if (cl == NULL) {
				cl = ice_check_list_new();
				ice_session_add_check_list(call->ice_session, cl);
				switch (stream->type) {
				case SalAudio:
					if (call->audiostream != NULL) call->audiostream->ms.ice_check_list = cl;
					break;
				case SalVideo:
					if (call->videostream != NULL) call->videostream->ms.ice_check_list = cl;
					break;
				default:
					break;
				}
			}
			if (stream->ice_mismatch == TRUE) {
				ice_check_list_set_state(cl, ICL_Failed);
			} else if (stream->rtp_port == 0) {
				//            } else if (stream->port == 0) {
				ice_session_remove_check_list(call->ice_session, cl);
			} else {
				if ((stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0'))
				{
					ice_check_list_set_remote_credentials(cl, stream->ice_ufrag, stream->ice_pwd);
				}
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES; j++) {
					const SalIceCandidate *candidate = &stream->ice_candidates[j];

					bool_t default_candidate = FALSE;
					const char *addr = NULL;
					int port = 0;
					if (candidate->addr[0] == '\0')
					{
						break;
					}
					if ((candidate->componentID == 0) || (candidate->componentID > 2))
					{
						continue;
					}
					get_default_addr_and_port(candidate->componentID, md, stream, &addr, &port);

					{
						PrintConsole("[DEBUG] remtoe addr = %s,port = %d\n",addr,port);
						PrintConsole("[DEBUG] remote candidate->addr = %s, candidate->port = %d\n",candidate->addr,candidate->port);
					}

					//                    sean modify
					if (addr && (candidate->port == port) && (strlen(candidate->addr) == strlen(addr)) && (strcmp(candidate->addr, addr) == 0))
						default_candidate = TRUE;
					//                    if (strcmp("101.36.88.2", candidate->addr)==0) {
					//                        default_candidate = TRUE;
					//                    }
					ice_add_remote_candidate(cl, candidate->type, candidate->addr, candidate->port, candidate->componentID,
						candidate->priority, candidate->foundation, default_candidate);
				}
				if (ice_restarted == FALSE) {
					bool_t losing_pairs_added = FALSE;
					for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES; j++) {
						const SalIceRemoteCandidate *candidate = &stream->ice_remote_candidates[j];
						const char *addr = NULL;
						int port = 0;
						int componentID = j + 1;
						if (candidate->addr[0] == '\0') break;
						get_default_addr_and_port(componentID, md, stream, &addr, &port);
						if (j == 0) {
							/* If we receive a re-invite and we finished ICE processing on our side, use the candidates given by the remote. */
							ice_check_list_unselect_valid_pairs(cl);
						}
						ice_add_losing_pair(cl, j + 1, candidate->addr, candidate->port, addr, port);
						losing_pairs_added = TRUE;
					}
					if (losing_pairs_added == TRUE) ice_check_list_check_completed(cl);
				}
			}
		}
		for (i = ice_session_nb_check_lists(call->ice_session); i > md->n_active_streams; i--) {
			ice_session_remove_check_list(call->ice_session, ice_session_check_list(call->ice_session, i - 1));
		}
		ice_session_check_mismatch(call->ice_session);
	}//end of if ((md->ice_pwd[0] != '\0') && (md->ice_ufrag[0] != '\0'))
	else {
		/* Response from remote does not contain mandatory ICE attributes, delete the session. */
		serphone_call_delete_ice_session(call);
		return;
	}
	if (ice_session_nb_check_lists(call->ice_session) == 0) {
		serphone_call_delete_ice_session(call);
	}
}

bool_t ServiceCore::serphone_core_media_description_contains_video_stream(const SalMediaDescription *md)
{
	int i;
	for (i = 0; i < md->n_active_streams; i++) {
		if (md->streams[i].type == SalVideo)
			return TRUE;
	}
	return FALSE;
}


void ServiceCore::serphone_call_start_media_streams_for_ice_gathering(SerPhoneCall *call)
{
	//	audio_stream_prepare_sound(call->audiostream, NULL, NULL);
	audio_stream_prepare_sound(call);
#ifdef VIDEO_ENABLED
	if (call->videostream) {
		video_stream_prepare_video(call->videostream);
	}
#endif
}

void ServiceCore::serphone_call_stop_media_streams_for_ice_gathering(SerPhoneCall *call)
{
	//	audio_stream_unprepare_sound(call->audiostream);
	audio_stream_unprepare_sound(call);
#ifdef VIDEO_ENABLED
	if (call->videostream) {
		video_stream_unprepare_video(call->videostream);
	}
#endif
}

int ServiceCore::serphone_core_gather_ice_candidates(/*SerphoneCore *lc, */SerPhoneCall *call)
{
	char local_addr_l[64];
	struct sockaddr_storage ss;
	socklen_t ss_len;
	IceCheckList *audio_check_list;
	IceCheckList *video_check_list;
	const char *server = serphone_core_get_stun_server();

	if ((server == NULL) || (call->ice_session == NULL)) return -1;
	audio_check_list = ice_session_check_list(call->ice_session, 0);
	video_check_list = ice_session_check_list(call->ice_session, 1);
	if (audio_check_list == NULL) return -1;

	if (this->sip_conf.ipv6_enabled){
		PrintConsole("stun support is not implemented for ipv6\n");
		return -1;
	}

	if (parse_hostname_to_addr(server, &ss, &ss_len) < 0) {
		PrintConsole("Fail to parser stun server address: %s\n", server);
		return -1;
	}
	if (this->vtable.display_status != NULL)
		this->vtable.display_status(this, _("ICE local candidates gathering in progress..."));

	/* Gather local host candidates. */
	if (serphone_core_get_local_ip_for(AF_INET, server, local_addr_l) < 0) {
		PrintConsole("Fail to get local ip\n");
		return -1;
	}

	strcpy(local_addr, local_addr_l);



	if ((ice_check_list_state(audio_check_list) != ICL_Completed) && (ice_check_list_candidates_gathered(audio_check_list) == FALSE)) {
		ice_add_local_candidate(audio_check_list, "host", local_addr_l, call->audio_port, 1, NULL);
		ice_add_local_candidate(audio_check_list, "host", local_addr_l, call->audio_port + 1, 2, NULL);
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateInProgress;

		//        //add customed candidate
		//        uint16_t componentID = 1;
		//        IceCandidate *candidate;
		//        MSList *base_elem;
		//        base_elem = ms_list_find_custom(audio_check_list->local_candidates, (MSCompareFunc)ice_find_host_candidate, &componentID);
		//        if (base_elem != NULL) {
		//            candidate = (IceCandidate *)base_elem->data;
		//            ice_add_local_candidate(audio_check_list, "srflx", "192.168.106.48", 8880, componentID, candidate);
		//            
		//        }
	}
	if (call->params.has_video && (video_check_list != NULL)
		&& (ice_check_list_state(video_check_list) != ICL_Completed) && (ice_check_list_candidates_gathered(video_check_list) == FALSE)) {
			ice_add_local_candidate(video_check_list, "host", local_addr_l, call->video_port, 1, NULL);
			ice_add_local_candidate(video_check_list, "host", local_addr_l, call->video_port + 1, 2, NULL);
			call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateInProgress;
	}



	PrintConsole("ICE: gathering candidate from [%s]\n",server);
	/* Gather local srflx candidates. */
	ice_session_gather_candidates(call->ice_session, ss, ss_len);
	return 0;
}

void ServiceCore::onStunPacket(const char* call_id, void*data,int len,const char *fromIP ,int fromPort, bool isRTCP, bool isVideo)
{
	//Construct remote addr
	sockaddr_in remote;
	int sockaddr_len = sizeof(struct sockaddr_in);
	memset(&remote, 0, sockaddr_len);
	remote.sin_family = AF_INET;
	remote.sin_port = ntohs(fromPort);
	remote.sin_addr.s_addr = inet_addr(fromIP);

	OrtpEvent *ev=ortp_event_new(ORTP_EVENT_STUN_PACKET_RECEIVED);
	OrtpEventData *ed=ortp_event_get_data(ev);
	ed->ep=rtp_endpoint_new((struct sockaddr *)&remote,sizeof(struct sockaddr));
	ed->info.socket_type = (isRTCP == 0)?OrtpRTPSocket:OrtpRTCPSocket;

	//Construct mb
	mblk_t *mb = (mblk_t *)malloc(sizeof(mblk_t));
	mb->b_rptr = (unsigned char *)data;
	mb->b_wptr = (unsigned char *)data+len;
	ed->packet = mb;
	mb->recv_addr.family = AF_INET;
	mb->recv_addr.addr.ipi_addr.s_addr = inet_addr(local_addr);

	eXosip_event_t *je;
	eXosip_event_init(&je, EXOSIP_STUN_PACKET);
	if(call_id)
		sprintf(je->call_id, "%s", call_id);
	je->stun_event = ev;
	je->is_video = isVideo;
	eXosip_event_add(je);
	return;
}

void  ServiceCore::onAudioData(const char *call_id, const void *inData, int inLen, void *outData, int &outLen, bool send)
{
	PrintConsole("DEBUG: onAudioData called\n");
	if (vtable.throw_data_2_process)
		vtable.throw_data_2_process(this,this->current_call,inData,inLen,outData,outLen,send);
#ifdef HAIYUNTONG
	if (NULL == inData) {
		return;
	}
	if (serphone_haiyuntong_enabled()) {
		if (send) {
			char *serverid = "server";
			char * encryptFactor = isAudioConf?serverid:remoteSipNo;
			serphone_encrypt((char *)inData, (long)inLen, encryptFactor, (long)strlen(encryptFactor), (char *)outData, (long *)&outLen);
		}
		else
		{
			char *serverid = "server";
			char *decryptFactor = isAudioConf?serverid:remoteSipNo;
			serphone_decrypt((char *)inData, (long)inLen, decryptFactor, (long)strlen(decryptFactor), (char *)outData, (long *)&outLen);
		}
	}
	else
	{
		outLen = inLen;
		memcpy(outData, inData, inLen);
	}

#endif
}

void  ServiceCore::onOriginalAudioData(const char *call_id, const void *inData, int inLen, int sampleRate, int numChannels, bool send)
{
	PrintConsole("DEBUG: onOriginalAudioData called\n");
	if (!send) {
		sampleRate = this->current_call->current_params.audio_codec->clock_rate;
		numChannels = this->current_call->current_params.audio_codec->channels;
	}

	if (vtable.throw_original_data_2_process)
		vtable.throw_original_data_2_process(this,this->current_call,inData,inLen,sampleRate,numChannels,send?"PCM":this->current_call->current_params.audio_codec->mime_type,send);
}


void ServiceCore::video_stream_iterate(VideoStream *stream)
{
	if (stream->ms.evq){
		OrtpEvent *ev;
		while (NULL != (ev=ortp_ev_queue_get(stream->ms.evq))) {
			OrtpEventType evt=ortp_event_get_type(ev);
			if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED){
				//				OrtpEventData *evd=ortp_event_get_data(ev);
				//				video_steam_process_rtcp(stream,evd->packet);
			}else if ((evt == ORTP_EVENT_STUN_PACKET_RECEIVED) && (stream->ms.ice_check_list)) {
				//				ice_handle_stun_packet(stream->ms.ice_check_list,stream->ms.session,ortp_event_get_data(ev));
			}
			ortp_event_destroy(ev);
		}
	}
	if (stream->ms.ice_check_list) ice_check_list_process(stream->ms.ice_check_list,stream->ms.session);
}

void ServiceCore::audio_stream_iterate(AudioStream *stream){

	if (stream->is_beginning && ms_time(NULL)-stream->start_time>15){
		rtp_session_set_rtcp_report_interval(stream->ms.session,5000);
		stream->is_beginning=FALSE;
	}
	if (stream->ms.evq){
		OrtpEvent *ev=ortp_ev_queue_get(stream->ms.evq);
		if (ev!=NULL){
			OrtpEventType evt=ortp_event_get_type(ev);
			if (evt==ORTP_EVENT_RTCP_PACKET_RECEIVED){
				//				audio_stream_process_rtcp(stream,ortp_event_get_data(ev)->packet);
				//				stream->last_packet_time=ms_time(NULL);
			}else if (evt==ORTP_EVENT_RTCP_PACKET_EMITTED){
				//				/*we choose to update the quality indicator when the oRTP stack decides to emit a RTCP report */
				//				if (stream->qi) ms_quality_indicator_update_local(stream->qi);
				//				PrintConsole("audio_stream_iterate(): local statistics available\n\tLocal's current jitter buffer size:%f ms",rtp_session_get_jitter_stats(stream->ms.session)->jitter_buffer_size_ms);
			}else if ((evt==ORTP_EVENT_STUN_PACKET_RECEIVED)&&(stream->ms.ice_check_list)){
				//				ice_handle_stun_packet(stream->ms.ice_check_list,stream->ms.session,ortp_event_get_data(ev));
			}
			ortp_event_destroy(ev);
		}
	}

	if (stream->ms.ice_check_list) ice_check_list_process(stream->ms.ice_check_list,stream->ms.session);
}

void ServiceCore::serphone_core_update_ice_state_in_call_stats(SerPhoneCall *call)
{
	IceCheckList *audio_check_list;
	IceCheckList *video_check_list;
	IceSessionState session_state;

	if (call->ice_session == NULL) return;
	audio_check_list = ice_session_check_list(call->ice_session, 0);
	video_check_list = ice_session_check_list(call->ice_session, 1);
	if (audio_check_list == NULL) return;

	session_state = ice_session_state(call->ice_session);
	if ((session_state == IS_Completed) || ((session_state == IS_Failed) && (ice_session_has_completed_check_list(call->ice_session) == TRUE))) {
		if (ice_check_list_state(audio_check_list) == ICL_Completed) {
			switch (ice_check_list_selected_valid_candidate_type(audio_check_list)) {
			case ICT_HostCandidate:
				call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateHostConnection;
				break;
			case ICT_ServerReflexiveCandidate:
			case ICT_PeerReflexiveCandidate:
				call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateReflexiveConnection;
				break;
			case ICT_RelayedCandidate:
				call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateRelayConnection;
				break;
			}
		} else {
			call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateFailed;
		}
		if (call->params.has_video && (video_check_list != NULL)) {
			if (ice_check_list_state(video_check_list) == ICL_Completed) {
				switch (ice_check_list_selected_valid_candidate_type(video_check_list)) {
				case ICT_HostCandidate:
					call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateHostConnection;
					break;
				case ICT_ServerReflexiveCandidate:
				case ICT_PeerReflexiveCandidate:
					call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateReflexiveConnection;
					break;
				case ICT_RelayedCandidate:
					call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateRelayConnection;
					break;
				}
			} else {
				call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateFailed;
			}
		}
	} else if (session_state == IS_Running) {
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateInProgress;
		if (call->params.has_video && (video_check_list != NULL)) {
			call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateInProgress;
		}
	} else {
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = SerPhoneIceStateFailed;
		if (call->params.has_video && (video_check_list != NULL)) {
			call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = SerPhoneIceStateFailed;
		}
	}
}

int ServiceCore::serphone_core_start_update_call(SerPhoneCall *call){
	const char *subject;
	call->camera_active=call->params.has_video;
	if (call->ice_session != NULL) {
		serphone_core_update_local_media_description_from_ice(call->localdesc, call->ice_session);
	}

	if (call->params.in_conference){
		subject="Conference";
	}else{
		subject="Media change";
	}
	if (this->vtable.display_status)
		this->vtable.display_status(this,_("Modifying call parameters..."));
	sal_call_set_local_media_description (call->op,call->localdesc);
	return sal_call_update(call->op,subject);
}

void ServiceCore::serphone_call_update_remote_session_id_and_ver(SerPhoneCall *call) {
	SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
	if (remote_desc) {
		call->remote_session_id = remote_desc->session_id;
		call->remote_session_ver = remote_desc->session_ver;
	}
}

int ServiceCore::serphone_core_start_accept_call_update(SerPhoneCall *call){
	SalMediaDescription *md;
	if (call->ice_session != NULL) {
		if (ice_session_nb_losing_pairs(call->ice_session) > 0) {
			/* Defer the sending of the answer until there are no losing pairs left. */
			return 0;
		}
		serphone_core_update_local_media_description_from_ice(call->localdesc, call->ice_session);
	}

	serphone_call_update_remote_session_id_and_ver(call);
	sal_call_set_local_media_description(call->op,call->localdesc);
	sal_call_accept(call->op);
	md=sal_call_get_final_media_description(call->op);
	if (md && !sal_media_description_empty(md))
		serphone_core_update_streams (call,md);
	serphone_call_set_state(call,LinphoneCallStreamsRunning,"Connected (streams running)");
	return 0;
}

int ServiceCore::serphone_core_proceed_with_invite_if_ready(SerPhoneCall *call, SerphoneProxyConfig *dest_proxy){
	bool_t ice_ready = FALSE;
	bool_t upnp_ready = FALSE;
	bool_t ping_ready = FALSE;

	if (call->ice_session != NULL) {
		if (ice_session_candidates_gathered(call->ice_session)) ice_ready = TRUE;
	} else {
		ice_ready = TRUE;
	}
#ifdef BUILD_UPNP
	if (call->upnp_session != NULL) {
		if (linphone_upnp_session_get_state(call->upnp_session) == LinphoneUpnpStateOk) upnp_ready = TRUE;
	} else {
		upnp_ready = TRUE;
	}
#endif //BUILD_UPNP
	if (call->ping_op != NULL) {
		if (call->ping_replied == TRUE) ping_ready = TRUE;
	} else {
		ping_ready = TRUE;
	}

	//	if ((ice_ready == TRUE) && (upnp_ready == TRUE) && (ping_ready == TRUE)) {
	if ((ice_ready == TRUE) && (ping_ready == TRUE)) {
		return serphone_core_start_invite(call, NULL);
	}
	return 0;
}

bool_t ServiceCore::serphone_core_incompatible_security(SalMediaDescription *md){
	if (serphone_core_is_media_encryption_mandatory() && serphone_core_get_media_encryption()==LinphoneMediaEncryptionSRTP){
		int i;
		for(i=0;i<md->n_active_streams;i++){
			SalStreamDescription *sd=&md->streams[i];
			if (sd->proto!=SalProtoRtpSavp){
				return TRUE;
			}
		}
	}
	return FALSE;
}

void ServiceCore::serphone_core_notify_incoming_call(SerPhoneCall *call){
	char *barmesg;
	char *tmp;
	SerphoneAddress *from_parsed;
	SalMediaDescription *md;
	bool_t propose_early_media=lp_config_get_int(this->config,"sip","incoming_calls_early_media",FALSE);
	const char *ringback_tone=serphone_core_get_remote_ringback_tone ();

	create_local_media_description(call);
	sal_call_set_local_media_description(call->op,call->localdesc);
	md=sal_call_get_final_media_description(call->op);
	if (md){
		if (sal_media_description_empty(md) || serphone_core_incompatible_security(md)){
			sal_call_decline(call->op,SalReasonMedia,NULL);
			serphone_call_unref(call);
			return;
		}
	}

	from_parsed=serphone_address_new(sal_op_get_from(call->op));
	serphone_address_clean(from_parsed);
	tmp=serphone_address_as_string(from_parsed);
	serphone_address_destroy(from_parsed);
	barmesg=ortp_strdup_printf("%s %s%s",tmp,_("is contacting you"),
		(sal_call_autoanswer_asked(call->op)) ?_(" and asked autoanswer."):_("."));
	if (this->vtable.show) this->vtable.show(this);
	if (this->vtable.display_status)
		this->vtable.display_status(this,barmesg);


	if(ms_list_size(this->calls) ==1) {
		PrintConsole("Incoming call, ringplay_flag=%d dmfs_playing_start_time=%d\n", this->m_ringplay_flag, this->dmfs_playing_start_time);
	} else {
		PrintConsole("Incoming call, more than one call\n");
		MSList *calls= this->calls;
		while(calls!= NULL){
			SerPhoneCall *call = (SerPhoneCall *)calls->data;
			PrintConsole("Incoming call, callid=%s\n", call->_user_call_id);
			calls=calls->next;
		}
	}

	/* play the ring if this is the only call*/
	if (ms_list_size(this->calls)==1){
		this->current_call=call;
		if (this->m_ringplay_flag && this->dmfs_playing_start_time!=0){
			this->ring_stop();
			this->m_ringplay_flag = FALSE;
			this->dmfs_playing_start_time = 0;
		}else{
			this->m_ringplay_flag = TRUE;
			this->ring_start(this->sound_conf.local_ring,2000);
		}
	}else{
		/* else play a tone within the context of the current call */
		call->ringing_beep=TRUE;
		this->serphone_core_play_tone();
	}

	serphone_call_set_state(call,LinphoneCallIncomingReceived,"Incoming call");

	if (call->state==LinphoneCallIncomingReceived){
		sal_call_notify_ringing(call->op,propose_early_media || ringback_tone!=NULL);

		if (propose_early_media || ringback_tone!=NULL){
			serphone_call_set_state(call,LinphoneCallIncomingEarlyMedia,"Incoming call early media");
			md=sal_call_get_final_media_description(call->op);
			serphone_core_update_streams(call,md);
		}
		if (sal_call_get_replaces(call->op)!=NULL && lp_config_get_int(this->config,"sip","auto_answer_replacing_calls",1)){
			serphone_core_accept_call(call);
		}
	}
	serphone_call_unref(call);

	ms_free((void **)&barmesg);
	ms_free((void **)&tmp);
}


void ServiceCore::handle_ice_events(SerPhoneCall *call, OrtpEvent *ev, void *data, bool isVideo){
	OrtpEventType evt=ortp_event_get_type(ev);
	OrtpEventData *evd=ortp_event_get_data(ev);
	int ping_time;

	if (evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) {
		switch (ice_session_state(call->ice_session)) {
		case IS_Completed:
			ice_session_select_candidates(call->ice_session);
			if (ice_session_role(call->ice_session) == IR_Controlling) {
				serphone_core_update_call(call, &call->current_params);
			}
			break;
		case IS_Failed:
			if (ice_session_has_completed_check_list(call->ice_session) == TRUE) {
				ice_session_select_candidates(call->ice_session);
				if (ice_session_role(call->ice_session) == IR_Controlling) {
					/* At least one ICE session has succeeded, so perform a call update. */
					serphone_core_update_call(call, &call->current_params);
				}
			}
			break;
		default:
			break;
		}
		serphone_core_update_ice_state_in_call_stats(call);
	} else if (evt == ORTP_EVENT_ICE_GATHERING_FINISHED) {

		if (evd->info.ice_processing_successful==TRUE) {
			ice_session_compute_candidates_foundations(call->ice_session);
			ice_session_eliminate_redundant_candidates(call->ice_session);
			ice_session_choose_default_candidates(call->ice_session);
			ping_time = ice_session_average_gathering_round_trip_time(call->ice_session);
			if (ping_time >=0) {
				call->ping_time=ping_time;
			}
		} else {
			PrintConsole("No STUN answer from [%s], disabling ICE",serphone_core_get_stun_server());
			serphone_call_delete_ice_session(call);
		}
		switch (call->state) {
		case LinphoneCallUpdating:
			serphone_core_start_update_call(call);
			break;
		case LinphoneCallUpdatedByRemote:
			serphone_core_start_accept_call_update(call);
			break;
		case LinphoneCallOutgoingInit:
			if (this->_terminate_call == 1) {
				return;
			}
			serphone_call_stop_media_streams_for_ice_gathering(call);
			serphone_core_proceed_with_invite_if_ready(call, NULL);
			break;
		case LinphoneCallIdle:
			serphone_call_stop_media_streams_for_ice_gathering(call);
			serphone_core_notify_incoming_call(call);
			break;
		default:
			break;
		}
	} else if (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) {
		serphone_core_start_accept_call_update(call);
		serphone_core_update_ice_state_in_call_stats(call);
	} else if (evt == ORTP_EVENT_ICE_RESTART_NEEDED) {
		ice_session_restart(call->ice_session);
		ice_session_set_role(call->ice_session, IR_Controlling);
		serphone_core_update_call(call, &call->current_params);
	} else if (evt==ORTP_EVENT_STUN_PACKET_RECEIVED){
		//evt==ORTP_EVENT_STUN_PACKET_RECEIVED
		/*add begin------------------Sean20130723----------for video ice------------*/

		//            sean add "call->videostream" to following to avoid EXEC_BAD_ACCESS 20140113 begin
		if (isVideo && call->videostream && call->videostream->ms.ice_check_list) {
			//            sean add "call->videostream" to following to avoid EXEC_BAD_ACCESS 20140113 end
			ice_handle_stun_packet(call->videostream->ms.ice_check_list,call->videostream->ms.session,ortp_event_get_data(ev),call);
		}
		/*add begin------------------Sean20130723----------for video ice------------*/
		//            sean add "call->audiostream" to following to avoid EXEC_BAD_ACCESS 20140113 begin
		else if (0 == isVideo && call->audiostream && call->audiostream->ms.ice_check_list)
		{
			//            sean add "call->audiostream" to following to avoid EXEC_BAD_ACCESS 20140113 end
			ice_handle_stun_packet(call->audiostream->ms.ice_check_list,call->audiostream->ms.session,ortp_event_get_data(ev),call);
		}
	}
}

void ServiceCore::serphone_call_background_tasks(SerPhoneCall *call, bool_t one_second_elapsed)
{
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	//    ServiceCore* lc = call->core;
	//    int disconnect_timeout = serphone_core_get_nortp_timeout();
	//	bool_t disconnected=FALSE;

#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL) {
		OrtpEvent *ev;

		/* Ensure there is no dangling ICE check list. */

		if (call->ice_session == NULL)
		{
			call->videostream->ms.ice_check_list = NULL;
		}

		// Beware that the application queue should not depend on treatments fron the
		// mediastreamer queue.
		video_stream_iterate(call->videostream);

		while (call->videostream_app_evq && (NULL != (ev=ortp_ev_queue_get(call->videostream_app_evq)))){
			OrtpEventType evt=ortp_event_get_type(ev);
			if ((evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) || (evt == ORTP_EVENT_ICE_GATHERING_FINISHED)
				|| (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) || (evt == ORTP_EVENT_ICE_RESTART_NEEDED)) {
					handle_ice_events(call, ev,this);
			}
			ortp_event_destroy(ev);
		}
	}
#endif
	if (call->audiostream!=NULL) {
		OrtpEvent *ev;

		/* Ensure there is no dangling ICE check list. */

		if (call->ice_session == NULL)
		{
			call->audiostream->ms.ice_check_list = NULL;
		}

		// Beware that the application queue should not depend on treatments fron the
		// mediastreamer queue.


		audio_stream_iterate(call->audiostream);

		while (call->audiostream_app_evq && (NULL != (ev=ortp_ev_queue_get(call->audiostream_app_evq)))){

			OrtpEventType evt=ortp_event_get_type(ev);
			if ((evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) || (evt == ORTP_EVENT_ICE_GATHERING_FINISHED)
				|| (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) || (evt == ORTP_EVENT_ICE_RESTART_NEEDED)) {
					handle_ice_events(call, ev,this);
			} else if (evt==ORTP_EVENT_TELEPHONE_EVENT){
				//				linphone_core_dtmf_received(lc,evd->info.telephone_event);
			}
			ortp_event_destroy(ev);
		}
	}
}


static void disable_checksums(ortp_socket_t sock) {
#if defined(DISABLE_CHECKSUMS) && defined(SO_NO_CHECK)
	int option = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &option, sizeof(option)) == -1) {
		ms_warning("Could not disable udp checksum: %s", strerror(errno));
	}
#endif
}

RtpSession * ServiceCore::create_duplex_rtpsession(int loc_rtp_port, int loc_rtcp_port, bool_t ipv6, int isVideo) {
	RtpSession *rtpr;

	rtpr = rtp_session_new(RTP_SESSION_SENDRECV);
	rtpr->session_type = isVideo;
	//	rtp_session_set_recv_buf_size(rtpr, MAX_RTP_SIZE);
	//	rtp_session_set_scheduling_mode(rtpr, 0);
	//	rtp_session_set_blocking_mode(rtpr, 0);
	//	rtp_session_enable_adaptive_jitter_compensation(rtpr, TRUE);
	//	rtp_session_set_symmetric_rtp(rtpr, TRUE);

	//	rtp_session_set_local_addr(rtpr, ipv6 ? "::" : "0.0.0.0", loc_rtp_port, loc_rtcp_port);
	//	rtp_session_signal_connect(rtpr, "timestamp_jump", (RtpCallback)rtp_session_resync, (long)NULL);
	//	rtp_session_signal_connect(rtpr, "ssrc_changed", (RtpCallback)rtp_session_resync, (long)NULL);
	//	rtp_session_set_ssrc_changed_threshold(rtpr, 0);
	//	rtp_session_set_rtcp_report_interval(rtpr, 2500);	/* At the beginning of the session send more reports. */
	//	disable_checksums(rtp_session_get_rtp_socket(rtpr));



	//    rtpr->rtp.sockfamily=sockfamily;
	rtpr->rtp.loc_port=loc_rtp_port;

	//             call->audiostream->ms.session->rtcp.sockfamily=sockfamily;
	//             call->audiostream->ms.session->rtcp.socket=sock;


	return rtpr;
}

AudioStream *ServiceCore::audio_stream_new(int loc_rtp_port, int loc_rtcp_port, bool_t ipv6, int channel){
	AudioStream *stream=(AudioStream *)ms_new0(AudioStream,1);

	stream->ms.type = AudioStreamType;
	stream->ms.session=create_duplex_rtpsession(loc_rtp_port,loc_rtcp_port,ipv6,AudioStreamType);
	stream->ms.session->channel = channel;
	/*some filters are created right now to allow configuration by the application before start() */
	stream->ms.ice_check_list=NULL;
	return stream;
}

VideoStream *ServiceCore::video_stream_new(int loc_rtp_port, int loc_rtcp_port, bool_t use_ipv6, int channel){
	VideoStream *stream = (VideoStream *)ms_new0 (VideoStream, 1);
	stream->ms.type = VideoStreamType;
	stream->ms.session=create_duplex_rtpsession(loc_rtp_port,loc_rtcp_port,use_ipv6,VideoStreamType);
	stream->ms.session->channel = channel;
	stream->ms.evq=ortp_ev_queue_new();
	stream->ms.ice_check_list=NULL;
	//	rtp_session_register_event_queue(stream->ms.session,stream->ms.evq);
	//	stream->sent_vsize.width=MS_VIDEO_SIZE_CIF_W;
	//	stream->sent_vsize.height=MS_VIDEO_SIZE_CIF_H;
	stream->dir=VideoStreamSendRecv;
	stream->display_filter_auto_rotate_enabled=0;
	//	choose_display_name(stream);

	return stream;
}


void ServiceCore::serphone_core_set_audio_pacinterval(int pacinterval)
{
    pacinterval = 20;
	PrintConsole("serphone_core_set_audio_pacinterval pacinterval=%d\n",pacinterval);
	m_packetInterval = pacinterval;
}

int ServiceCore::serphone_core_get_audio_pacinterval()
{
	return m_packetInterval;
}

int ServiceCore::serphone_core_reset_audio_device()
{
#if !defined(NO_VOIP_FUNCTION)
	if( serphone_core_get_calls_nb() <= 0 )
		return -1; 
	ECMedia_reset_audio_device();
#endif
	return 0;
}

void ServiceCore::serphone_core_set_dtx_enabled(bool_t enabled)
{
	m_dtxEnabled = enabled;
}

int ServiceCore::serphone_core_start_rtp_dump(SerPhoneCall *call, int mediatype, const char* file, cloopenwebrtc::RTPDirections direction)
{
#if !defined(NO_VOIP_FUNCTION)
	if(!call)
		return -1;
	
	int ret = -1;
	if(mediatype == 0)
	{
		ret = ECMedia_start_rtp_dump(call->m_AudioChannelID, false, file, direction);
	}
#ifdef VIDEO_ENABLED
	else
	{
		ret = ECMedia_start_rtp_dump(call->m_VideoChannelID, true, file, direction);
	}
#endif
	return ret;

#endif
	return -1;
}

int ServiceCore::serphone_core_stop_rtp_dump(SerPhoneCall *call, int mediatype, cloopenwebrtc::RTPDirections direction)
{
#if !defined(NO_VOIP_FUNCTION)
	if(!call)
		return -1;

	int ret = -1;
	if(mediatype == 0)
	{
		ret = ECMedia_stop_rtp_dump(call->m_AudioChannelID, false, direction);
	}
#ifdef VIDEO_ENABLED
	else
	{
		ret = ECMedia_stop_rtp_dump(call->m_VideoChannelID, true, direction);
	}
#endif
	return ret;

#endif
	return -1;
}

void ServiceCore::serphone_core_set_video_bitrates(int bitrates)
{
	PrintConsole("serphone_core_set_video_bitrates video bitrates=%d\n",bitrates);
	m_videoBitRates = bitrates;
}

int ServiceCore::getPlayoutDeviceInfo(SpeakerInfo** speakerinfo)
{
#if !defined(NO_VOIP_FUNCTION)
	PrintConsole("start getPlayoutDeviceInfo");
	bool mediaRelease = false;

	if(m_speakerInfo)
		delete[] m_speakerInfo;
	if( ECMedia_get_playout_device_num(m_speakerCount) < 0) {
		return 0;
	}
	m_speakerInfo = new SpeakerInfo[m_speakerCount];
	for(int i=0; i<m_speakerCount; i++)
	{
		char strName[128], strGuid[128];
		if( ECMedia_get_specified_playout_device_info(i, strName, strGuid) == 0 )
		{
			m_speakerInfo[i].index = i;
			strcpy(m_speakerInfo[i].name, strName);
			strcpy(m_speakerInfo[i].guid, strGuid);
		}
	}

	*speakerinfo = m_speakerInfo;
	PrintConsole("end getPlayoutDeviceInfo");
	return m_speakerCount;
#else
	return 0;
#endif
}

int ServiceCore::selectPlayoutDevice(int index)
{
#if !defined(NO_VOIP_FUNCTION)
	if(!m_speakerInfo)
	{
		PrintConsole("there is no Speaker Info, can't select");
		return -1;
	}

	if(index < 0  || index >= m_speakerCount)
		return -1;

	if(index == m_usedSpeakerIndex)
		return 0;

	SerPhoneCall *call =serphone_core_get_current_call();
	if( call!= NULL )
	{
		ECMedia_select_playout_device(index);
	}

	m_usedSpeakerIndex = index;
#endif
	return 0;
}

int ServiceCore::getRecordDeviceInfo(MicroPhoneInfo** microphoneinfo)
{
#if !defined(NO_VOIP_FUNCTION)
	PrintConsole("getRecordDeviceInfo");
	bool mediaRelease = false;

	if(m_microphoneInfo)
		delete[] m_microphoneInfo;

	if( ECMedia_get_record_device_num(m_microphoneCount) < 0) {
		return 0;
	}
	m_microphoneInfo = new MicroPhoneInfo[m_microphoneCount];
	for(int i=0; i<m_microphoneCount; i++)
	{
		char strName[128], strGuid[128];
		if( ECMedia_get_specified_record_device_info(i, strName, strGuid) == 0 )
		{
			m_microphoneInfo[i].index = i;
			strcpy(m_microphoneInfo[i].name, strName);
			strcpy(m_microphoneInfo[i].guid, strGuid);
		}
	}

	*microphoneinfo = m_microphoneInfo;
	PrintConsole("getRecordDeviceInfo");

	return m_microphoneCount;
#else
	return 0;
#endif
}

int ServiceCore::selectRecordDevice(int index)
{
#if !defined(NO_VOIP_FUNCTION)
	if(!m_microphoneInfo)
	{
		PrintConsole("there is no MicroPhone Info, can't select");
		return -1;
	}

	if(index < 0  || index >= m_microphoneCount)
		return -1;

	if(index == m_usedMicrophoneIndex)
		return 0;

	SerPhoneCall *call =serphone_core_get_current_call();
	if( call!= NULL)
	{
		ECMedia_select_record_device(index);
	}
	m_usedMicrophoneIndex = index;
#endif
	return 0;
}
int ServiceCore::startDeliverVideoFrame(SerPhoneCall *call)
{
#ifdef VIDEO_ENABLED
	if(!call || call->m_VideoChannelID < 0)
	{
		PrintConsole("startDeliverVideoFrame failed, call is not ready!\n");
		return -1;
	}
	//TODO:
	//PrintConsole("startDeliverVideoFrame  call->m_CaptureDeviceId=%d\n", call->m_CaptureDeviceId);
	//if(call->deliver_frame) {
	//	delete call->deliver_frame;
	//	call->deliver_frame = NULL;
	//}
	//call->deliver_frame = new VideoFrameDeliver(call);

	//if(m_vie)
	//{
	//	ViECapture *capture = ViECapture::GetInterface(m_vie);
	//	return capture->RegisterFrameCallback(call->m_CaptureDeviceId, call->deliver_frame);
	//}
#endif
	return -1;

}

int ServiceCore::stopDeliverVideoFrame(SerPhoneCall *call)
{
#ifdef VIDEO_ENABLED
	if(!call || call->m_VideoChannelID < 0 /*|| !call->deliver_frame*/)
	{
		PrintConsole("stopDeliverVideoFrame failed, call is not ready!\n");
		return -1;
	}

	//TODO:
	//if(m_vie)
	//{
	//	ViECapture *capture = ViECapture::GetInterface(m_vie);
	//	int ret = capture->DeregisterFrameCallback(call->m_CaptureDeviceId, call->deliver_frame);
	//	delete call->deliver_frame;
	//	call->deliver_frame = NULL;

	//	return ret;
	//}
#endif
	return -1;
}
void ServiceCore::DeliverFrameData(SerPhoneCall *call, unsigned char*buf, int size, int width, int height)
{
#ifdef VIDEO_ENABLED
	if(this->vtable.deliver_video_frame)
		this->vtable.deliver_video_frame(this, call, buf, size, width, height);
#endif
	return;
}

int ServiceCore::getLocalVideoSnapshot(SerPhoneCall *call, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
#ifdef VIDEO_ENABLED
	if(!call || call->m_VideoChannelID < 0)
	{
		PrintConsole("getLocalVideoSnapshot failed, call is not ready!\n");
		return -1;
	}
	return ECMedia_get_local_video_snapshot(call->m_CaptureDeviceId, buf, size, width, height);
#endif
	return -1;
}

int ServiceCore::getLocalVideoSnapshot(SerPhoneCall *call, const char* filePath)
{
#ifdef VIDEO_ENABLED
	if(!call || call->m_VideoChannelID < 0)
	{
		PrintConsole("getLocalVideoSnapshot failed, call is not ready!\n");
		return -1;
	}
	return ECMedia_save_local_video_snapshot(call->m_CaptureDeviceId, filePath);
#endif
	return -1;
}

int ServiceCore::getRemoteVideoSnapshot(SerPhoneCall *call, unsigned char **buf, unsigned int *size, unsigned int *width, unsigned int *height)
{
#ifdef VIDEO_ENABLED
	if(!call || call->m_VideoChannelID < 0)
	{
		PrintConsole("getRemoteVideoSnapshot failed, call is not ready!\n");
		return -1;
	}
	return ECMedia_get_remote_video_snapshot(call->m_VideoChannelID, buf, size, width, height);
#endif
	return -1;
}

int ServiceCore::getRemoteVideoSnapshot(SerPhoneCall *call, const char* filePath)
{
#ifdef VIDEO_ENABLED
	if(!call || call->m_VideoChannelID < 0)
	{
		PrintConsole("getRemoteVideoSnapshot failed, call is not ready!\n");
		return -1;
	}
	return ECMedia_save_remote_video_snapshot(call->m_VideoChannelID, filePath);
#endif
	return -1;
}

//Sean add begin 20131022 for video fast update in video conference
int ServiceCore::serphone_send_key_frame(SerPhoneCall *call)
{
#ifdef VIDEO_ENABLED
	ECMedia_send_key_frame(call->m_VideoChannelID);
	return 0;
#endif
	return -1;
}
//Sean add end 20131022 for video fast update in video conference

int ServiceCore::serphone_call_start_record_audio(SerPhoneCall *call, const char *filename)
{
#if !defined(NO_VOIP_FUNCTION)

	if(!capability_conf.localrec) {
		return -3;
	}

	if(!call) {
		return -1;
	}

	//TODO:
	//if(!call->record_voip) {
	//	PrintConsole("serphone_call_start_record_audio\n");

	//	call->record_voip = new RecordVoip();
	//}

	//if(call->record_voip->isStartRecordWav()) {
	//	serphone_call_stop_record_audio(call);
	//}

	//int ret = call->record_voip->StartRecordAudio(filename);
	//if(m_voe && call->m_AudioChannelID >= 0) {
	//	PrintConsole("RegisterExternalMediaProcessin in serphone_call_start_record_audio\n");
	//	VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
	//	if(exmedia) {
	//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kPlaybackPerChannel, *call->record_voip);
	//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kRecordingPerChannel, *call->record_voip);
	//		exmedia->Release();
	//	}
	//}
	//return ret;

#endif
	return 0;
}
int ServiceCore::serphone_call_stop_record_audio(SerPhoneCall *call)
{
#if !defined(NO_VOIP_FUNCTION)

	if(!capability_conf.localrec) {
		return -3;
	}

	//TODO:
	//if(!call || !call->record_voip)
	//	return -1;

	//if(m_voe && !(call->record_voip->isStartRecordMp4() || call->record_voip->isStartRecordScree()) ) {
	//	VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
	//	if(exmedia) {
	//		exmedia->DeRegisterExternalMediaProcessing(call->m_AudioChannelID,  kPlaybackPerChannel);
	//		exmedia->DeRegisterExternalMediaProcessing(call->m_AudioChannelID,  kRecordingPerChannel);
	//		exmedia->Release();
	//	}
	//}

	//call->record_voip->StopRecordAudio(0);
#endif
	return 0;
}

int ServiceCore::record_audio_status(SerPhoneCall *call, const char *filename, int status)
{
#if !defined(NO_VOIP_FUNCTION)
	if(this->vtable.record_audio_callback) {
		this->vtable.record_audio_callback(this, call, filename, status);
	}
#endif
	return 0;
}

int ServiceCore::serphone_call_start_record_audio_ex(SerPhoneCall *call, const char *rFileName, const char *lFileName)
{
#if !defined(NO_VOIP_FUNCTION)

	//if(!capability_conf.localrec) {
	//	return -3;
	//}

	if(!call) {
		return -1;
	}

	//TODO:
	//if(!call->record_voip) {
	//	PrintConsole("serphone_call_start_record_audio\n");
	//	call->record_voip = new RecordVoip();
	//}

	//if(call->record_voip->isStartRecordWav()) {
	//	serphone_call_stop_record_audio(call);
	//}

	//int ret = call->record_voip->StartRecordAudioEx(rFileName, lFileName);

	//if(m_voe && call->m_AudioChannelID >= 0) {
	//	PrintConsole("RegisterExternalMediaProcessin in serphone_call_start_record_audio\n");
	//	VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
	//	if(exmedia) {
	//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kPlaybackPerChannel, *call->record_voip);
	//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kRecordingPerChannel, *call->record_voip);
	//		exmedia->Release();
	//	}
	//}
	//return ret;

#endif
	return 0;
}

int ServiceCore::serphone_call_start_record_voip(SerPhoneCall *call, const char *filename)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED

	if(!capability_conf.localrecvoip) {
		return -3;
	}  

	if(!call) {
		return -1;
	}
	//if(!call->record_voip) {
	//	PrintConsole("serphone_call_start_record_voip\n");
	//	call->record_voip = new RecordVoip();
	//}

	//if(call->record_voip->isStartRecordMp4()) {
	//	serphone_call_stop_record_voip(call);
	//}

	//int ret = call->record_voip->StartRecordVoip(filename);

	//if(m_voe && call->m_AudioChannelID >= 0) {
	//	PrintConsole("RegisterExternalMediaProcessin in serphone_call_start_record_audio\n");
	//	VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
	//	if(exmedia) {
	//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kPlaybackPerChannel, *call->record_voip);
	//		exmedia->RegisterExternalMediaProcessing(call->m_AudioChannelID,  kRecordingPerChannel, *call->record_voip);
	//	}
	//	exmedia->Release();
	//}

	//if(m_vie && call->m_VideoChannelID >= 0) {
	//	ViEFile *file = ViEFile::GetInterface(m_vie);
	//	if(file) {
	//		file->RegisterVideoFrameStorageCallBack(call->m_VideoChannelID, call->record_voip);
	//		ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
	//		if(rtp_rtcp) {
	//			rtp_rtcp->RequestKeyFrame(call->m_VideoChannelID);
	//			rtp_rtcp->Release();
	//		}
	//	}
	//	file->Release();
	//}

	//return ret;
#endif
#endif

	return -1;
}

int ServiceCore::serphone_call_stop_record_voip(SerPhoneCall *call)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED

	if(!capability_conf.localrecvoip) {
		return -3;
	}

	//if(!call || !call->record_voip)
	//	return -1;

	//if( m_voe && !(call->record_voip->isStartRecordWav() || call->record_voip->isStartRecordScree()) ) {
	//	VoEExternalMedia* exmedia = VoEExternalMedia::GetInterface(m_voe);
	//	if(exmedia) {
	//		exmedia->DeRegisterExternalMediaProcessing(call->m_AudioChannelID,  kPlaybackPerChannel);
	//		exmedia->DeRegisterExternalMediaProcessing(call->m_AudioChannelID,  kRecordingPerChannel);
	//		exmedia->Release();
	//	}
	//}
	//if(m_vie && call->m_VideoChannelID >= 0) {
	//	ViEFile *file = ViEFile::GetInterface(m_vie);
	//	if(file) {
	//		file->RegisterVideoFrameStorageCallBack(call->m_VideoChannelID, NULL);
	//		ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
	//		if(rtp_rtcp) {
	//			rtp_rtcp->RequestKeyFrame(call->m_VideoChannelID);
	//			rtp_rtcp->Release();
	//		}
	//	}
	//	file->Release();
	//}

	//return call->record_voip->StopRecordVoip(0);
#endif
#endif
	return 0;
}

int ServiceCore::record_voip_status(SerPhoneCall *call, const char *filename, int status)
{
	PrintConsole("ServiceCore::record_voip_status file=%s, status=%d\n", filename, status);
	return 0;
}

int ServiceCore::serphone_call_start_record_screen(SerPhoneCall *call, const char *filename, int bitrates, int fps, int type)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED

	if(!capability_conf.localrecvoip) {
		return -3;
	}

	if(!call) {
		return -1;
	}
	return ECMedia_start_record_screen(call->m_AudioChannelID, filename, bitrates, fps, type);
#endif
#endif
	return -1;
}
int ServiceCore::serphone_call_stop_record_screen(SerPhoneCall *call)
{
#ifdef VIDEO_ENABLED

	if(!capability_conf.localrec) {
		return -3;
	}

	if(!call)
		return -1;

	return ECMedia_stop_record_screen(call->m_AudioChannelID);
#endif
	return 0;
}
//
//int ServiceCore::serphone_call_record_screen_snapshot(SerPhoneCall *call, unsigned char*image, int size, int width, int height)
//{	
//	if(!call || !call->record_voip) {
//		return -1;
//	}
//	if(call->record_voip->isStartRecordScree()) {
//		call->record_voip->CapturedScreeImage(image, size, width, height);
//	}
//
//	return 0;
//}


//Sean add begin 20131119 noise suppression
int ServiceCore::serphone_noise_suppression(const void* audioSamples,
	WebRtc_Word16 *out)
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef WEBRTC_MAC
	ECMedia_init_audio();
#endif
	//TODO:
	//VoEBase *tempVoEBase = VoEBase::GetInterface(m_voe);
	//if (tempVoEBase) {
	//	int ret = tempVoEBase->NoiseSuppression(audioSamples, out);
	//	tempVoEBase->Release();
	//	return ret;
	//}
#endif
	return -1;
}
//Sean add end 20131119 noise suppression

//Sean add begin 20140322 for windows XP
int ServiceCore::serphone_soft_mute(SerPhoneCall *call, bool mute)
{
	int ret = -1;
#ifndef NO_VOIP_FUNCTION
	//TODO:
	//VoEBase *tempVoEBase = VoEBase::GetInterface(m_voe);
	//if (tempVoEBase) {
	//	if (!call) {
	//		tempVoEBase->Release();
	//		PrintConsole("ServiceCore::serphone_soft_mute invalid call which is NULL\n");
	//		return -1;
	//	}
	//	ret = tempVoEBase->pause(call->m_AudioChannelID, mute);
	//	tempVoEBase->Release();
	//}

	if (0 == ret) {
		softMuteStatus = mute;
	}
	return ret;
#endif
	return ret;
}


bool ServiceCore::serphone_soft_get_mute_status(SerPhoneCall *call)
{
#ifndef NO_VOIP_FUNCTION
	return softMuteStatus;
#endif
	return false;
}
//Sean add begin 20140322 for windows XP

int ServiceCore::serphone_call_get_network_statistic(SerPhoneCall *call,  long long *duration, long long *sendTotalSim, long long *recvTotalSim, long long *sendTotalWifi, long long *recvTotalWifi)
{
	time_t voe_time = 0;
	time_t vie_time = 0;
	long long voe_send_total_sim =0;
	long long voe_recv_total_sim = 0;
	long long vie_send_total_sim = 0;
	long long vie_recv_total_sim = 0;

	long long voe_send_total_wifi =0;
	long long voe_recv_total_wifi = 0;
	long long vie_send_total_wifi = 0;
	long long vie_recv_total_wifi = 0;

	long long voe_duration = 0;
	long long vie_duration = 0;

#if !defined(NO_VOIP_FUNCTION)
	if(call->m_AudioChannelID >= 0) {
		//TODO:
		//VoENetwork *network = VoENetwork::GetInterface(m_voe);
		//if(network){
		//	network->getNetworkStatistic(call->m_AudioChannelID, voe_time, voe_send_total_sim, voe_recv_total_sim, voe_send_total_wifi, voe_recv_total_wifi);
		//	network->Release();
		//}
	}

#ifdef VIDEO_ENABLED
	if(call->m_VideoChannelID >= 0) {
		//TODO:
		//ViENetwork *network = ViENetwork::GetInterface(m_vie);
		//if(network){
		//	network->getNetworkStatistic(call->m_VideoChannelID, vie_time, vie_send_total_sim, vie_recv_total_sim, vie_send_total_wifi, vie_recv_total_wifi);
		//	network->Release();
		//}
	}
#endif
	time_t  curaaa = time(NULL);
	if(voe_time) {
		voe_duration = time(NULL) - voe_time;
	}
	if(vie_time) {
		vie_duration = time(NULL) - vie_time;
	}
	*duration = voe_duration>vie_duration ? voe_duration:vie_duration;
	*sendTotalSim = voe_send_total_sim  + vie_send_total_sim ;
	*sendTotalWifi = voe_send_total_wifi + vie_send_total_wifi;
	*recvTotalSim = voe_recv_total_sim + vie_recv_total_sim;
	*recvTotalWifi = voe_recv_total_wifi + vie_recv_total_wifi;
	return 0;

#endif

	return -1;
}

int ServiceCore::serphone_call_reset_video_views(SerPhoneCall *call, void* remoteView,void *localView)
{
#if !defined(NO_VOIP_FUNCTION)
	if(!call) {
		return -1;
	}
#ifdef VIDEO_ENABLED
	if(remoteView) {

#ifdef WIN32
		RECT rect;
		::GetWindowRect((HWND)remoteView, &rect);
		int tmpSize = (rect.right-rect.left) * (rect.bottom-rect.top);

		if( (remoteView != videoWindow) || (tmpSize > videoWindowSize*1.2  ||  tmpSize < videoWindowSize*0.8) )
		{
			ECMedia_stop_render(call->m_VideoChannelID, -1);
			videoWindowSize = tmpSize;
			ECMedia_add_render(call->m_VideoChannelID, videoWindow, return_video_width_height);
		}
#endif
		videoWindow = remoteView;
	}

	if(localView) {
#ifdef WIN32
		RECT rect;
		::GetWindowRect((HWND)localView, &rect);
		int tmpSize = (rect.right-rect.left) * (rect.bottom-rect.top);

		if( (localView != localVideoWindow) || (tmpSize > localVideoWindowSize*1.2  ||  tmpSize < localVideoWindowSize*0.8) )
		{
			localVideoWindowSize = tmpSize;
			ECMedia_stop_render(call->m_CaptureDeviceId, -1);
			videoWindowSize = tmpSize;
			ECMedia_add_render(call->m_CaptureDeviceId, videoWindow, return_video_width_height);
		}
#else
		//capture->SetLocalVideoWindow(call->m_CaptureDeviceId,localVideoWindow);
#endif 

		localVideoWindow = localView;
	}
	return 0;
#endif
#endif
	return -1;
}

//sean add begin 20140422 SetAudioGain
int ServiceCore::serphone_set_audio_gain(float inaudio_gain, float outaudio_gain)
{
	int ret = -1;
#ifndef NO_VOIP_FUNCTION
	//TODO:
	//VoEBase *tempVoEBase = VoEBase::GetInterface(m_voe);
	//if (tempVoEBase) {
	//	if (inaudio_gain > 1e-08) {
	//		ret = tempVoEBase->setEnlargeAudioFlagIncoming(true, inaudio_gain);
	//	}
	//	else
	//	{
	//		ret = tempVoEBase->setEnlargeAudioFlagIncoming(false, inaudio_gain);
	//	}
	//	if (-1 == ret) {
	//		PrintConsole("ServiceCore::serphone_set_audio_gain set inaudio_gain error\n");
	//		tempVoEBase->Release();
	//		return ret;
	//	}
	//	if (inaudio_gain > 1e-08) {
	//		ret = tempVoEBase->setEnlargeAudioFlagOutgoing(true, outaudio_gain);
	//	}
	//	else
	//	{
	//		ret = tempVoEBase->setEnlargeAudioFlagOutgoing(false, outaudio_gain);
	//	}
	//	if (-1 == ret) {
	//		PrintConsole("ServiceCore::serphone_set_audio_gain set outaudio_gain error\n");
	//		tempVoEBase->Release();
	//		return ret;
	//	}
	//	tempVoEBase->Release();
	//}
#endif
	return ret;
}
//sean add end 20140422 SetAudioGain

int ServiceCore::serphone_core_set_speaker_volume(unsigned int volume)
{
#if !defined(NO_VOIP_FUNCTION)
	if(volume >255)
		return -1;

	speaker_volume = volume;
	return ECMedia_set_speaker_volume(volume);
#endif
	return -1;
}
unsigned int ServiceCore::serphone_core_get_speaker_volume()
{
	return speaker_volume;
}

//sean add begin 20140626 init and release audio device
int ServiceCore::serphone_register_audio_device()
{
	int ret = -1;
#ifndef NO_VOIP_FUNCTION
	//VoEBase *tempVoEBase = VoEBase::GetInterface(m_voe);
	//if (tempVoEBase) {
	//	ret = tempVoEBase->RegisterAudioDevice();
	//	tempVoEBase->Release();
	//}
#endif
	return ret;
}

int ServiceCore::serphone_deregister_audio_device()
{
	int ret = -1;
#ifndef NO_VOIP_FUNCTION
	//VoEBase *tempVoEBase = VoEBase::GetInterface(m_voe);
	//if (tempVoEBase) {
	//	ret = tempVoEBase->DeRegisterAudioDevice();
	//	tempVoEBase->Release();
	//}
#endif
	return ret;
}
//sean add end 20140626 init and release audio device


//sean add begin 20140616 video conference
int ServiceCore::serphone_set_video_conference_addr(const char *ip)
{
	int ret = -1;
#ifdef VIDEO_ENABLED
	int ipLen = strlen(ip);
	if (!videoConferenceIp) {
		videoConferenceIp = (char *)malloc(ipLen+1);
		if (!videoConferenceIp)
		{
			PrintConsole("ERROR: serphone_set_video_conference_addr mem alloc error\n");
			return -1;
		}
	}
	else if (strlen(videoConferenceIp) < ipLen)
	{
		videoConferenceIp = (char *)realloc(videoConferenceIp, ipLen+1);
		if (!videoConferenceIp) {
			PrintConsole("ERROR: serphone_set_video_conference_addr mem alloc error\n");
			return -1;
		}
	}

	memset(videoConferenceIp, 0, ipLen+1);
	memcpy(videoConferenceIp, ip, ipLen);
	videoConferenceIp[ipLen] = '\0';
	//    videoConferencePort = port;
	ret = 0;
#endif
	return ret;

}

int ServiceCore::serphone_set_video_window_and_request_video_accord_sip(const char *sipNo, void *videoWindowC, const char *conferenceNo, const char *confPasswd, int port)
{
	int ret = -1;
	PrintConsole("[WARNING] %s called\n",__FUNCTION__);
#ifdef VIDEO_ENABLED
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	if (!sipNo) {
		PrintConsole("[ERROR] request video failed, sip no is null, check it!\n");
		return -1;
	}
	if (!videoWindowC) {
		PrintConsole("[ERROR] request video failed, video window is null, check it!\n");
		return -2;
	}
	if (!conferenceNo) {
		PrintConsole("[ERROR] request video failed, conferenceNo is null, check it!\n");
		return -3;
	}
	if (!confPasswd) {
		PrintConsole("[ERROR] request video failed, confPasswd is null, check it!\n");
		return -4;
	}
	if (!selfSipNo) {
		PrintConsole("[ERROR] request video failed, self sip is null, check it!\n");
		return -5;
	}
	if (!videoConferenceIp) {
		PrintConsole("[ERROR] request video failed, video conference ip is null, check it!\n");
		return -6;
	}

	memset(conferenceID, 0, 100);
	memcpy(conferenceID, conferenceNo, strlen(conferenceNo));
	memset(conferencePsw, 0, 100);
	memcpy(conferencePsw, confPasswd, strlen(confPasswd));

	//First check whether the specified sip has been requested
	bool reinviteFlag = false;
	int temp_video_channel_id = -1;
	std::map<std::string, int>::iterator it = videoConferencePairSipChannel.find(sipNo);
	if (it != videoConferencePairSipChannel.end()) {
		PrintConsole("[WARNNING] you have got %s video media!\n",sipNo);
		reinviteFlag = true;
		temp_video_channel_id = it->second;

	}

	VideoConferenceDesc *temp = NULL;
	if (!reinviteFlag) {
		temp = (VideoConferenceDesc *)malloc(sizeof(VideoConferenceDesc));
		CursorVideoConferencePort += 2;
		temp->local_port = CursorVideoConferencePort;
		int sipLen = strlen(sipNo);
		temp->remoteSip = new char[sipLen+1];
		memcpy(temp->remoteSip, sipNo, sipLen);
		temp->remoteSip[sipLen] = '\0';
		ECMedia_audio_create_channel(temp_video_channel_id, true);
		//network->setVideoConferenceFlag(temp_video_channel_id, selfSipNo, sipNo, conferenceNo, confPasswd);
		videoConferenceM.insert(std::pair <int,VideoConferenceDesc *> (temp_video_channel_id, temp));
		videoConferencePairSipChannel.insert(std::pair<const char *, int>(sipNo,temp_video_channel_id));
		ECMedia_video_set_send_destination(temp_video_channel_id, videoConferenceIp,port, port+1);
		ECMedia_video_set_local_receiver(temp_video_channel_id,temp->local_port);
		//TODO:
		//network->RegisterServiceCoreCallBack(temp_video_channel_id, this, NULL, LinphonePolicyNoFirewall);
		ECMedia_video_start_receive(temp_video_channel_id);
	}
	else
	{

		//reuse the original resource
		std::map<int,VideoConferenceDesc *>::iterator itt = videoConferenceM.find(it->second);
		temp = itt->second;
		if (temp->conference_state == Video_Conference_State_Canceling) {
			PrintConsole("[ERROR] %s is stopping, wait!",sipNo);
			return -8;
		}
	}
	//    rtp_rtcp->SetNACKStatus(temp_video_channel_id, true);
	ECMedia_set_RTCP_status_video(temp_video_channel_id, 2/*kRtcpNonCompound_RFC5506*/);
	temp->video_window = videoWindowC;
	temp->is_waiting = true;
	temp->request_time = time(NULL);
	temp->request_status = -1;

	temp->server_port = port;
	temp->conference_state = Video_Conference_State_Nomal;
	//    request video after succeeding doing this
	//    build request body
	//     [ client_id:??￠??·??ˉSIP??·???, conf_id:???è????·???, member_id:èˉ·?±?????‘????SIP??·???,conf_pass:???è???ˉ????]
	char *data = new char[512];
	memset(data, 0, 512);

	int cursor = 0;
	memcpy(data, "yuntongxunyt", 12);
	cursor = 12;
	*(data+cursor) = '[';
	cursor += 1;
	int clientKeyLen = strlen("client_id:");
	memcpy(data+cursor, "client_id:", clientKeyLen);
	cursor += clientKeyLen;
	int selfSipLen = strlen(selfSipNo);
	memcpy(data+cursor, selfSipNo, selfSipLen);
	cursor += selfSipLen;

	*(data+cursor) = ',';
	cursor++;

	int confKeyLen = strlen("conf_id:");
	memcpy(data+cursor, "conf_id:", confKeyLen);
	cursor += confKeyLen;
	int confNoLen = strlen(conferenceNo);
	memcpy(data+cursor, conferenceNo, confNoLen);
	cursor += confNoLen;

	*(data+cursor) = ',';
	cursor++;

	int memberIDKeyLen = strlen("member_id:");
	memcpy(data+cursor, "member_id:", memberIDKeyLen);
	cursor += memberIDKeyLen;
	int memberValueLen = strlen(sipNo);
	memcpy(data+cursor, sipNo, memberValueLen);
	cursor += memberValueLen;

	*(data+cursor) = ',';
	cursor++;

	int conKeyLen = strlen("conf_pass:");
	memcpy(data+cursor, "conf_pass:", conKeyLen);
	cursor += conKeyLen;
	int conValueLen = strlen(confPasswd);
	memcpy(data+cursor, confPasswd, conValueLen);
	cursor += conValueLen;
	*(data+cursor) = ',';
	cursor++;

	int typeKeyLen = strlen("req_type:");
	memcpy(data+cursor, "req_type:", typeKeyLen);
	cursor += typeKeyLen;
	memcpy(data+cursor, "1", 1);
	cursor += 1;
	*(data+cursor) = ']';
	cursor++;

	if (videoConferenceIp) {
		int transmitted_bytes;
		ECMedia_sendUDPPacket(temp_video_channel_id, data, cursor, transmitted_bytes, false, port, videoConferenceIp);
		ECMedia_setVideoConferenceFlag(temp_video_channel_id, selfSipNo, sipNo, conferenceNo, confPasswd, port, videoConferenceIp);
		//Send RTCP
		data[cursor-2] = '3';
		ECMedia_sendUDPPacket(temp_video_channel_id, data, cursor, transmitted_bytes, true, port+1, videoConferenceIp);
		temp->conference_state = Video_Conference_State_Requesting;		
		ret = 0;
	} else {
		ret = -8;
	}
	delete [] data;
	ret = 0;
#endif
	return ret;
}
//sean add end 20140616 video conference

int ServiceCore::serphone_stop_conference_video_accord_sip(const char *sipNo, const char *conferenceNo, const char *confPasswd)
{
	PrintConsole("[WARNING] %s called\n",__FUNCTION__);
	int ret = -1;
#ifdef VIDEO_ENABLED
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	if (!sipNo) {
		PrintConsole("[ERROR] request video failed, sip no is null, check it!\n");
		return -1;
	}
	if (!conferenceNo) {
		PrintConsole("[ERROR] request video failed, conferenceNo is null, check it!\n");
		return -3;
	}
	if (!confPasswd) {
		PrintConsole("[ERROR] request video failed, confPasswd is null, check it!\n");
		return -4;
	}
	if (!selfSipNo) {
		PrintConsole("[ERROR] request video failed, self sip is null, check it!\n");
		return -5;
	}
	if (!videoConferenceIp) {
		PrintConsole("[ERROR] request video failed, video conference ip is null, check it!\n");
		return -6;
	}
	//First check whether the specified sip has been requested
	std::map<std::string, int>::iterator it = videoConferencePairSipChannel.find(sipNo);
	if (it == videoConferencePairSipChannel.end()) {
		PrintConsole("[WARNNING] you haven't got %s video media!\n",sipNo);
		return -7;
	}

	int temp_video_channel_id = it->second;
	VideoConferenceDesc *tempVideoConfDesc = NULL;
	std::map<int, VideoConferenceDesc*>::iterator iter = videoConferenceM.find(temp_video_channel_id);
	if (iter != videoConferenceM.end()) {
		tempVideoConfDesc = iter->second;
	} else {
		PrintConsole("[ERROR] Cannot find specified video conference description according to channel id : %d\n",temp_video_channel_id);
		return -7;
	}

	char *data = new char[512];
	memset(data, 0, 512);

	int cursor = 0;
	memcpy(data, "yuntongxunyt", 12);
	cursor = 12;
	*(data+cursor) = '[';
	cursor += 1;
	int clientKeyLen = strlen("client_id:");
	memcpy(data+cursor, "client_id:", clientKeyLen);
	cursor += clientKeyLen;
	int selfSipLen = strlen(selfSipNo);
	memcpy(data+cursor, selfSipNo, selfSipLen);
	cursor += selfSipLen;

	*(data+cursor) = ',';
	cursor++;

	int confKeyLen = strlen("conf_id:");
	memcpy(data+cursor, "conf_id:", confKeyLen);
	cursor += confKeyLen;
	int confNoLen = strlen(conferenceNo);
	memcpy(data+cursor, conferenceNo, confNoLen);
	cursor += confNoLen;

	*(data+cursor) = ',';
	cursor++;

	int memberIDKeyLen = strlen("member_id:");
	memcpy(data+cursor, "member_id:", memberIDKeyLen);
	cursor += memberIDKeyLen;
	int memberValueLen = strlen(sipNo);
	memcpy(data+cursor, sipNo, memberValueLen);
	cursor += memberValueLen;

	*(data+cursor) = ',';
	cursor++;

	int conKeyLen = strlen("conf_pass:");
	memcpy(data+cursor, "conf_pass:", conKeyLen);
	cursor += conKeyLen;
	int conValueLen = strlen(confPasswd);
	memcpy(data+cursor, confPasswd, conValueLen);
	cursor += conValueLen;
	*(data+cursor) = ',';
	cursor++;

	int typeKeyLen = strlen("req_type:");
	memcpy(data+cursor, "req_type:", typeKeyLen);
	cursor += typeKeyLen;
	memcpy(data+cursor, "0", 1);
	cursor += 1;
	*(data+cursor) = ']';
	cursor++;

	if (videoConferenceIp) {
		PrintConsole("[DEBUG] %s send cancel oder on channel:%d\n",__FUNCTION__,temp_video_channel_id);
		int transmitted_bytes;
		ECMedia_sendUDPPacket(temp_video_channel_id, data, cursor, transmitted_bytes, false,tempVideoConfDesc->server_port, videoConferenceIp);
		tempVideoConfDesc->conference_state = Video_Conference_State_Canceling;
		ret = 0;
	} else {
		ret = -8;
		//todo
	}
	delete [] data;

	//Release resource
	//    std::map<std::string,int>::iterator it2 = videoConferencePairSipChannel.find(tempVideoConfDesc->remoteSip);
	//    if( it2!=videoConferencePairSipChannel.end() )
	//    {
	//        videoConferencePairSipChannel.erase(it2);
	//    }

	if (0 == tempVideoConfDesc->request_status) {
		ECMedia_stop_render(temp_video_channel_id, -1);
	}

	ECMedia_video_stop_receive(temp_video_channel_id);
	ECMedia_delete_channel(temp_video_channel_id, true);

	delete [] tempVideoConfDesc->remoteSip;
	free(tempVideoConfDesc);
	videoConferenceM.erase(iter);
	videoConferencePairSipChannel.erase(it);
	//    base->Release();
#endif
	return ret;
}
//sean add begin 20140705 video conference
void ServiceCore::onVideoConference(int channelID, int status, int payload)
{
	PrintConsole("[DEBUG] %s called\n",__FUNCTION__);
#ifdef VIDEO_ENABLED
	VideoConferenceDesc *tempVideoConfDesc = NULL;
	std::map<int, VideoConferenceDesc*>::iterator it = videoConferenceM.find(channelID);
	if (it != videoConferenceM.end())
	{
		tempVideoConfDesc = it->second;
	}
	else
	{
		PrintConsole("[ERROR] Cannot find specified video conference description according to channel id : %d\n",channelID);
		return;
	}
	PrintConsole("[DEBUG] in  %s conference_state:%d\n",__FUNCTION__,tempVideoConfDesc->conference_state);
	if(tempVideoConfDesc->conference_state == Video_Conference_State_Requesting) {
		switch (status) {
		case Video_Conference_status_OK:
			{
				cloopenwebrtc::VideoCodec codec_params;
				bool codec_found = false;
				int num_codec = ECMedia_num_of_supported_codecs_video();
				cloopenwebrtc::VideoCodec *codecArray = new cloopenwebrtc::VideoCodec[num_codec];
				for (int i = 0; i < num_codec; i++) {
					codec_params = codecArray[i];
					if ( strcasecmp( codec_params.plName,"H264" ) == 0) {
						codec_found = true;
						codec_params.plType = payload;
						break;
					}
				}
				delete []codecArray;

				if (codec_found) {
					if(m_videoBitRates > 0 && m_videoBitRates > codec_params.minBitrate) {
						codec_params.startBitrate = m_videoBitRates;
					}
					PrintConsole("Video Codec is : playload type = %d, payload name is %s  bitrate=%d width=%d height=%d\n",codec_params.plType, codec_params.plName, codec_params.startBitrate, codec_params.width, codec_params.height);
					ECMedia_set_receive_codec_video(channelID,codec_params);
				}
				ECMedia_add_render(channelID,tempVideoConfDesc->video_window,return_video_width_height);

				tempVideoConfDesc->conference_state = Video_Conference_State_Streaming;
			}
			break;
		case Video_Conference_status_NotExist:
		case Video_Conference_status_UserExclusive:
		case Video_Conference_status_RequestedUserExclusive:
		case Video_Conference_status_RequestedUserNoVideo:
			{
			}
			break;
		default:
			PrintConsole("[ERROR] VideoConference default\n");
			break;
		}
		tempVideoConfDesc->request_status = status;
		tempVideoConfDesc->is_waiting = false;
	}
	else if( tempVideoConfDesc->conference_state ==Video_Conference_State_Canceling)
	{
		PrintConsole("[DEBUG] %s cancel response\n",__FUNCTION__);
		if (vtable.stop_specified_video_response) {
			vtable.stop_specified_video_response(this,this->current_call,tempVideoConfDesc->remoteSip, status, tempVideoConfDesc->video_window);
		}

		switch (status) {
		case Video_Conference_status_OK:
			{

			}
			break;
		case Video_Conference_status_NotExist:
		case Video_Conference_status_UserExclusive:
		case Video_Conference_status_RequestedUserExclusive:
		case Video_Conference_status_RequestedUserNoVideo:
			{
			}
			break;
		default:
			PrintConsole("[ERROR] VideoConference default\n");
			break;
		}

		//        std::map<std::string,int>::iterator it2 = videoConferencePairSipChannel.find(tempVideoConfDesc->remoteSip);
		//        if( it2!=videoConferencePairSipChannel.end() )
		//        {
		//            videoConferencePairSipChannel.erase(it2);
		//        }
		//        
		//        if (0 == tempVideoConfDesc->request_status) {
		//            ViERender* render =  ViERender::GetInterface(m_vie);
		//            render->StopRender(channelID);
		//            //#ifdef _WIN32
		//            //            render->StopRender(captureID);
		//            //#endif
		//            render->Release();
		//        }
		//        
		//        ViEBase* base = ViEBase::GetInterface(m_vie);
		//        base->StopReceive(channelID);
		//        base->DeleteChannel(channelID);
		//        base->Release();
		//        
		//        delete [] tempVideoConfDesc->remoteSip;
		//        free(tempVideoConfDesc);
		//        videoConferenceM.erase(it);
	}

#endif
}

int ServiceCore::serphone_set_video_conference_released()
{
	PrintConsole("[DEBUG] %s called\n",__FUNCTION__);
#ifdef VIDEO_ENABLED
	cloopenwebrtc::CriticalSectionScoped lock(m_criticalSection);
	VideoConferenceDesc *tempVideoConfDesc = NULL;
	std::map<std::string, int>::iterator it1 = videoConferencePairSipChannel.begin();
	while (it1 != videoConferencePairSipChannel.end()) {
		serphone_stop_conference_video_accord_sip(it1->first.c_str(), conferenceID, conferencePsw);
		//        videoConferencePairSipChannel.erase(it1);
		it1 = videoConferencePairSipChannel.begin();
	}
	std::map<int, VideoConferenceDesc*>::iterator it2 = videoConferenceM.begin();
	while (it2 != videoConferenceM.end()) {
		tempVideoConfDesc = it2->second;
		int channelID = it2->first;

		if (0 == it2->second->request_status) {
			ECMedia_stop_render(channelID, -1);
		}

		ECMedia_video_stop_receive(channelID);
		ECMedia_delete_channel(channelID, true);

		delete [] tempVideoConfDesc->remoteSip;
		free(tempVideoConfDesc);
		videoConferenceM.erase(it2);
		it2 = videoConferenceM.begin();
	}
	CursorVideoConferencePort = serphone_core_get_video_port();
	return 0;
#endif
	return -1;
}

int ServiceCore::serphone_reset_conference_video_window(const char *sipNo, void *newVideoWindowC)
{
	PrintConsole("[DEBUG] %s called\n",__FUNCTION__);
#ifdef VIDEO_ENABLED
	std::map<std::string , int>::iterator it1 = videoConferencePairSipChannel.find(sipNo);
	if (it1 != videoConferencePairSipChannel.end()) {
		//ok, we get it
		VideoConferenceDesc *temp = NULL;
		PrintConsole("[DEBUG] look up sip:%s, channel id:%d\n",sipNo,it1->second);

		std::map<int, VideoConferenceDesc *>::iterator it2 = videoConferenceM.find(it1->second);
		if (it2 != videoConferenceM.end()) {
			//ok we get the video conference desc
			temp = it2->second;
			temp->video_window = newVideoWindowC;
			//restart video display
			ECMedia_stop_render(it2->first, -1);

			ECMedia_add_render(it2->first,newVideoWindowC,return_video_width_height);
		}
		else
		{
			PrintConsole("[WARNING] speciafied sip %s not exists, videoConferenceM\n",sipNo);
			return -4;
		}
	}
	else
	{
		PrintConsole("[WARNING] speciafied sip %s not exists, videoConferencePairSipChannel\n",sipNo);
		return -4;
	}
	return 0;
#endif
	return -1;
}

void ServiceCore::serphone_check_video_conference_request_failed()
{
#ifdef VIDEO_ENABLED
	std::map<int, VideoConferenceDesc *>::iterator it = videoConferenceM.begin();
	for (; it != videoConferenceM.end();) {
		std::map<int, VideoConferenceDesc *>::iterator it_back = it;
		bool is_first_element = false;
		if (it_back != videoConferenceM.begin()) {
			it_back--;
		}
		else
			is_first_element = true;

		time_t curr = time(NULL);

		if ((it->second->is_waiting && (curr - it->second->request_time > 5)) || (false == it->second->is_waiting && it->second->request_status != Video_Conference_status_OK)) {
			//request time out
			//delete item and report
			videoConferencePairSipChannel.erase(it->second->remoteSip);
			//report
			if (it->second->is_waiting && curr - it->second->request_time > 5) {
				it->second->request_status = Video_Conference_status_RequestTimeout;
			}
			if (vtable.request_specified_video_failed) {
				vtable.request_specified_video_failed(this,this->current_call,it->second->remoteSip, it->second->request_status);
			}
			//Release resource
			int channelID = it->first;

			ECMedia_video_stop_receive(channelID);
			ECMedia_delete_channel(channelID, true);

			delete [] it->second->remoteSip;
			free(it->second);
			videoConferenceM.erase(it);
			if (is_first_element) {
				it = videoConferenceM.begin();
			}
			else
				it = ++it_back;
		}
		else
			it++;
	}
#endif
}

int ServiceCore::serphone_set_silk_rate(int rate)
{
	if (rate < 5000 || rate > 20000) {
		return -1;
	}
	m_silkBitRate = rate;
	return 0;
}
//sean add end 20140705 video conference

int ServiceCore::PlayAudioFromRtpDump(int localPort, const char *ptName, int ploadType)
{
#if !defined(NO_VOIP_FUNCTION)
#ifndef WIN32
	ECMedia_init_audio();
#endif
	
	ECMedia_audio_create_channel(m_AudioChannelIDDump, false);

	ECMedia_set_local_receiver(m_AudioChannelIDDump, localPort, localPort+1);

	cloopenwebrtc::CodecInst codec_params = {0};
	bool codec_found = false;

	int codec_num = ECMedia_num_of_supported_codecs_audio();
	cloopenwebrtc::CodecInst *codecArray = new cloopenwebrtc::CodecInst[codec_num];
	ECMedia_get_supported_codecs_audio(codecArray);
	for (int i = 0; i < codec_num; i++) {
		codec_params = codecArray[i];
		if ( strcasecmp( codec_params.plname, ptName) == 0) {
			codec_found = true;
			codec_params.pltype = ploadType;
			break;
		}
	}
	delete []codecArray;

	if (codec_found) {        
		ECMedia_set_receive_playloadType_audio(m_AudioChannelIDDump,codec_params);
	}	

	ECMedia_audio_start_receive(m_AudioChannelIDDump);
	ECMedia_audio_start_playout(m_AudioChannelIDDump);

#endif
	return 0;
}

int ServiceCore::StopPlayAudioFromRtpDump()
{
#if !defined(NO_VOIP_FUNCTION)
	ECMedia_audio_stop_playout(m_AudioChannelIDDump);
	ECMedia_audio_stop_receive(m_AudioChannelIDDump);
	ECMedia_delete_channel(m_AudioChannelIDDump,false);

#ifndef WIN32
	ECMedia_uninit_audio();
#endif

#endif
	return 0;
}

int ServiceCore::PlayVideoFromRtpDump(int localPort, const char *ptName, int ploadType, void *videoWindow)
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef VIDEO_ENABLED

	ECMedia_audio_create_channel(m_VideoChannelIDDump, true);
	ECMedia_video_set_local_receiver(m_VideoChannelIDDump, localPort, localPort+1);
	ECMedia_set_MTU(m_VideoChannelIDDump,1450);


	if( videoWindow ) {
#ifdef WIN32
		RECT remoteRect;
		::GetWindowRect((HWND)videoWindow, &remoteRect);
		videoWindowSize = (remoteRect.right-remoteRect.left) * (remoteRect.bottom-remoteRect.top);
#endif
		ECMedia_add_render(m_VideoChannelIDDump,videoWindow,return_video_width_height);
	}

	PrintConsole("ENABLE_REMB_TMMBR_CONFIG: videoNackEnabled=%d  true, \n", videoNackEnabled);
	ECMedia_set_NACK_status_video(m_VideoChannelIDDump, videoNackEnabled);
	ECMedia_set_RTCP_status_video(m_VideoChannelIDDump, 2/*kRtcpNonCompound_RFC5506*/);


	cloopenwebrtc::VideoCodec codec_params;
	bool codec_found = false;
	int codec_num = ECMedia_num_of_supported_codecs_video();
	cloopenwebrtc::VideoCodec *codecArray = new cloopenwebrtc::VideoCodec[codec_num];
	ECMedia_get_supported_codecs_video(codecArray);
	for (int i = 0; i < codec_num; i++) {
		codec_params = codecArray[i];
		if ( strcasecmp( codec_params.plName, ptName) == 0) {
			codec_found = true;
			codec_params.plType = ploadType;
			break;
		}
	}

	{
		codec_params.width = 1440;
		codec_params.height = 900;
		codec_params.maxFramerate = 15;
	}
	if (codec_found) {
		ECMedia_set_receive_codec_video(m_VideoChannelIDDump,codec_params);
	}
	ECMedia_video_start_receive(m_VideoChannelIDDump);

//#ifdef	ENABLE_RECORD_RAW_VIDEO
//	{
//		ViEFile *file_record = ViEFile::GetInterface(m_vie);
//		CodecInst audio_codec;
//		memcpy(codec_params.plName, "I420", kPayloadNameSize);
//		file_record->StartRecordIncomingVideo(m_VideoChannelIDDump, "incoming_video.avi", AudioSource::NO_AUDIO,audio_codec, codec_params);
//		//file_record->StartRecordOutgoingVideo(m_VideoChannelIDDump, "outgoing_video.avi", AudioSource::NO_AUDIO, audio_codec, codec_params);
//		file_record->Release();
//	}		
//#endif
#endif
#endif
	return 0;
}

int ServiceCore::StopPlayVideoFromRtpDump()
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef VIDEO_ENABLED
	//TODO:
	//if(!m_vie){
	//	return -1;
	//}	
	//ViERender* render =  ViERender::GetInterface(m_vie);
	//render->StopRender(m_VideoChannelIDDump);
	//render->Release();

	//ViEBase* base = ViEBase::GetInterface(m_vie);
	//base->StopReceive(m_VideoChannelIDDump);
	//base->DeleteChannel(m_VideoChannelIDDump);
	//base->Release();

	//ViEExternalCodec *extCodec = ViEExternalCodec::GetInterface(m_vie);
	//extCodec->DeRegisterExternalReceiveCodec(m_VideoChannelIDDump, 98);
	//extCodec->Release();
#endif
#endif
	return 0;
}

int ServiceCore::startVideoWithoutCall(int cameraIndex, int videoW, int videoH, int rotate, void *videoWnd)
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef VIDEO_ENABLED
	//TODO:
//	if(!m_vie) {
//		PrintConsole("startVideoCapture failed. m_vie =%0x\n", m_vie);
//		return -1001;
//	}
//
//	if(m_SnapshotDeviceId != -1) {
//		PrintConsole("startVideoCapture failed. already captured call->m_CaptureDeviceId=%d\n",m_SnapshotDeviceId);
//		return -1002;
//	}
//
//	ViEBase* base = ViEBase::GetInterface(m_vie);
//	ViECapture *capture = ViECapture::GetInterface(m_vie);
//
//	base->CreateChannel(m_SnapshotChannelID);
//
//	if(!m_cameraInfo) {
//		CameraInfo *info;
//		getCameraInfo(&info);
//	}
//
//	if(cameraIndex < 0 || cameraIndex >= m_cameraCount) {
//		PrintConsole("startVideoCapture failed. already captured call->m_CaptureDeviceId=%d\n",m_SnapshotDeviceId);
//		capture->Release();
//		base->Release();
//		return -1003;
//	}
//
//	char name[256], id[256];
//	if ( capture->GetCaptureDevice(cameraIndex,name,sizeof(name),id,sizeof(id)) < 0)  {
//		PrintConsole("Can not find video device \n");
//		capture->Release();
//		base->Release();
//		return -1004;
//	} else {
//		capture->AllocateCaptureDevice(id,strlen(id),m_SnapshotDeviceId);
//		if( capture->ConnectCaptureDevice(m_SnapshotDeviceId, m_SnapshotChannelID) < 0 ) {
//			PrintConsole("Open Camera:%s Failed!  \n", name);
//			capture->Release();
//			base->Release();
//			return -1005;
//		}
//
//		CaptureCapability cap;
//		cap.height = videoW; //cc.height;
//		cap.width = videoH;//cc.width;
//		cap.maxFPS = 30;
//
//		m_SnapshotDstWidth = videoW;
//		m_SnapshotDstHeight = videoH;
//
//		RotateCapturedFrame tr = (RotateCapturedFrame)m_camerRotate;
//		if(m_camerRotate == -1)  {
//			capture->GetOrientation(id,tr);
//		}
//
//		capture->SetRotateCapturedFrames(m_SnapshotDeviceId,tr);
//		capture->StartCapture(m_SnapshotDeviceId,cap);
//
//		PrintConsole("Use No %d camera:%s,height:%d,width:%d,framerate:%d,roate=%d \n",
//			m_usedCameraIndex,name,cap.height,cap.width,cap.maxFPS,tr);
//
//		if( videoWnd ) {
//#ifdef WIN32
//			ViERender* render =  ViERender::GetInterface(m_vie);
//			render->AddRenderer(NULL,m_SnapshotDeviceId,videoWnd,1,0,0,1,1,NULL);
//			render->StartRender(m_SnapshotDeviceId);						
//			render->Release();
//#else
//			capture->SetLocalVideoWindow(m_SnapshotDeviceId,videoWnd);
//#endif
//		}
//	}
//
//	capture->Release();
//	base->Release();
	return 0;
#endif
#endif
	return -1;
}

int ServiceCore::stopVideoWithoutCall()
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef VIDEO_ENABLED
//TODO:
//	if (!m_vie)
//	{
//		PrintConsole("video_stream_stop m_vie==NULL\n");
//		return -1001;
//	}
//
//	if (m_SnapshotChannelID>=0) {
//		if( m_SnapshotDeviceId >= 0)  {
//			ViECapture *capture = ViECapture::GetInterface(m_vie);
//			capture->StopCapture(m_SnapshotDeviceId);
//			capture->ReleaseCaptureDevice(m_SnapshotDeviceId);
//			capture->Release();
//		}
//
//		ViERender* render =  ViERender::GetInterface(m_vie);
//		render->StopRender(m_SnapshotChannelID);
//#ifdef _WIN32
//		render->StopRender(m_SnapshotDeviceId);
//#endif
//		render->Release();
//
//		ViEBase* base = ViEBase::GetInterface(m_vie);
//		base->DeleteChannel(m_SnapshotChannelID);
//		base->Release();
//	}
//	m_SnapshotChannelID = -1;
//	m_SnapshotDeviceId = -1;
	return 0;
#endif
#endif
	return -1;
}


int ServiceCore::getSnapshotWithoutCall(const char *filePath)
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef VIDEO_ENABLED

	if(m_SnapshotDeviceId < 0)
	{
		PrintConsole("getLocalVideoSnapshot failed, call is not ready!\n");
		return -1;
	}
	//TODO:
	//ViEFile *file = ViEFile::GetInterface(m_vie);
	//if(!file) {
	//	return -1;
	//}
	//if(file) {

	//	int getPicTimes = 3;
	//	while(getPicTimes>0) {
	//		if(  file->GetCaptureDeviceSnapshot(m_SnapshotDeviceId, filePath) >= 0)
	//			break;
	//		getPicTimes--;
	//	}
	//	if(getPicTimes <= 0) {
	//		file->Release();
	//		return -1;
	//	}
	//	file->Release();
	//	return 0;
	//}

#endif
#endif
	return -1;
}


void ServiceCore::serphone_core_restart_nack(SerPhoneCall *call)
{
#if !defined(NO_VOIP_FUNCTION)
	//TODO:
	//if(call && call->resultdesc)
	//{
	//	/* look for savp stream first */
	//	const SalStreamDescription *stream=sal_media_description_find_stream(call->resultdesc,
	//		SalProtoRtpSavp,SalAudio);
	//	/* no savp audio stream, use avp */
	//	if (!stream)
	//		stream=sal_media_description_find_stream(call->resultdesc, SalProtoRtpAvp,SalAudio);

	//	VoERTP_RTCP *rtp_rtcp = VoERTP_RTCP::GetInterface(m_voe);
	//	if(rtp_rtcp ) {
	//		rtp_rtcp->SetNACKStatus(call->m_AudioChannelID, false, 450);

	//		if(stream && stream->nack_support)

	//			rtp_rtcp->SetNACKStatus(call->m_AudioChannelID, false, 450);

	//		rtp_rtcp->Release();
	//	}
	//}
#endif
}

#ifdef VIDEO_ENABLED
//void ServiceCore::BrightnessAlarm(const int capture_id, const cloopenwebrtc::Brightness brightness)
//{
//	//PrintConsole("BrightnessAlarm capture_id=%d, brightness=%d\n", capture_id, brightness);
//}
//void ServiceCore::CapturedFrameRate(const int capture_id, const unsigned char frame_rate)
//{
//	//PrintConsole("CapturedFrameRate capture_id=%d, frame_rate=%d\n", capture_id, frame_rate);
//}
//void ServiceCore::NoPictureAlarm(const int capture_id, const cloopenwebrtc::CaptureAlarm alarm)
//{
//	PrintConsole("NoPictureAlarm capture_id=%d, alarm=%d\n", capture_id, alarm);
//	if(this->current_call && this->current_call->m_CaptureDeviceId == capture_id) {
//		if(vtable.video_capture_status) {
//			vtable.video_capture_status(this, this->current_call, alarm);
//		}
//	}
//}

//// This method will be called periodically delivering a deada€?ora€?alive
//// decision for a specified channel.
//void ServiceCore::OnPeriodicDeadOrAlive(const int video_channel, const bool alive)
//{
//
//}
//// This method is called once if a packet timeout occurred.
//void ServiceCore::PacketTimeout(const int video_channel, const cloopenwebrtc::ViEPacketTimeout type)
//{
//	if(vtable.video_packet_timeout) {
//		vtable.video_packet_timeout(this, this->current_call, type);
//	}
//}
#endif

int ServiceCore::FixedCameraInfo(const char *cameraName, const char *cameraId, int width, int height)
{
#if !defined(NO_VOIP_FUNCTION)

#ifdef VIDEO_ENABLED
	CameraInfo *info = NULL; 
	int cameraCount = getCameraInfo(&info);
	if( cameraCount <= 0) {
		return -1;
	}
	if(!cameraName  && !cameraId) {
		return -1;
	}
	m_usedCameraIndex = -1;

	if(width > 0 && height > 0) {
		m_sendVideoWidth = width;
		m_sendVideoHeight = height;
	}

	bool foundCamera = false;
	bool foundCapability = false;
	int maxFPS = 0;
	for(int i=0; i<cameraCount; i++) {
		if(cameraName && strlen(cameraName) && !strcmp(cameraName, info[i].name)){
			foundCamera = true;
		}
		if(cameraId && strlen(cameraId)) {
			if(!strcmp(cameraId, info[i].id) ) {
				foundCamera = true;
			}else {
				foundCamera = false;
			}
		}

		for(int j=0;  j < info[i].capabilityCount && foundCamera;  j++) {
#ifdef WEBRTC_ANDROID 
			if(m_sendVideoWidth == info[i].capability[j].height && m_sendVideoHeight == info[i].capability[j].width 
				&& info[i].capability[j].maxfps > maxFPS) {
#else
			if(m_sendVideoWidth == info[i].capability[j].width && m_sendVideoHeight == info[i].capability[j].height 
				&& info[i].capability[j].maxfps > maxFPS) {
#endif
					foundCapability = true;
					m_usedCameraIndex = i;
					m_usedCapabilityIndex = j;
			}
		}
	}
	if(foundCamera && foundCapability) 
		return 0;

#endif
#endif
	return -1;
}

int ServiceCore::ConfigureChromaKey(const char *bgImage, float angle, float noise_level, int r, int g, int b)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	//TODO:
	//if(!m_ChromaKeyFilter) {
	//	m_ChromaKeyFilter = new ChromaKeyFilter();
	//	if(!m_ChromaKeyFilter)
	//	{
	//		PrintConsole("Alloc ChromakeyFilter failed.");
	//		return -1;
	//	}
	//}
	//m_ChromaKeyFilter->SetEffectAngle(angle);
	//m_ChromaKeyFilter->SetEffectNoiseLevel(noise_level);
	//if( r>=0 && r<=255 && g>=0 && g<=255 && b>=0 && b<=255)
	//	m_ChromaKeyFilter->SetKeyColor(r, g, b);
	//return m_ChromaKeyFilter->SetBackGroundImage((WebRtc_Word8*)bgImage);
#endif
#endif
	return -1;
}

int ServiceCore::StartVirtualBackGround()
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	//TODO:
	//if(!m_vie)
	//	return -1;

	//if(!m_ChromaKeyFilter) {
	//	PrintConsole("ChromaKeyFilter has't init,, start Virtual background failed.");
	//	return -1;
	//}

	//if(m_ChromaKeyFilter->IsChromaKeyStart()) {
	//	return 0;
	//}

	//m_ChromaKeyFilter->StartChromaKey();
	//SerPhoneCall *call =serphone_core_get_current_call();
	//if(call) {
	//	ViEImageProcess *imageprocess = ViEImageProcess::GetInterface(m_vie);
	//	if(imageprocess) {
	//		imageprocess->RegisterCaptureEffectFilter(call->m_CaptureDeviceId, *m_ChromaKeyFilter);
	//	}
	//}
	return 0;
#endif
#endif
	return -1;
}

int ServiceCore::StopVirtualBakcGround()
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	//TODO:
	//if(!m_vie || !m_ChromaKeyFilter)
	//	return -1;

	//if(!m_ChromaKeyFilter->IsChromaKeyStart()) {
	//	return 0;
	//}

	//m_ChromaKeyFilter->StopChromaKey();
	//SerPhoneCall *call =serphone_core_get_current_call();
	//if(call) {
	//	ViEImageProcess *imageprocess = ViEImageProcess::GetInterface(m_vie);
	//	if(imageprocess) {
	//		imageprocess->DeregisterCaptureEffectFilter(call->m_CaptureDeviceId);
	//	}
	//}
	return 0;   
#endif
#endif
	return -1;
}

int ServiceCore::GetBandwidthUsage(const char* callid, 
	unsigned int& total_bitrate_sent,
	unsigned int& video_bitrate_sent,
	unsigned int& fec_bitrate_sent,
	unsigned int& nackBitrateSent)
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if (!call)
	{
		return -1;
	}

	//TODO:
	//ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);

	//if (!rtp_rtcp)
	//{
	//	return -1;
	//}

	//rtp_rtcp->GetBandwidthUsage(call->m_VideoChannelID, total_bitrate_sent, video_bitrate_sent, fec_bitrate_sent, nackBitrateSent);
	//rtp_rtcp->Release();
	return 0;
}

int ServiceCore::GetEstimatedSendBandwidth( const char* callid, 
	                                         unsigned int* estimated_bandwidth)
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if (!call)
	{
		return -1;
	}

	//TODO:
	//ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);

	//if (!rtp_rtcp)
	//{
	//	return -1;
	//}

	//rtp_rtcp->GetEstimatedSendBandwidth(call->m_VideoChannelID, estimated_bandwidth);
	//rtp_rtcp->Release();
	return 0;
}

int  ServiceCore::GetEstimatedReceiveBandwidth(const char* callid, 
	                             unsigned int* estimated_bandwidth)
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if (!call)
	{
		return -1;
	}

	//TODO:
	//ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);

	//if (!rtp_rtcp)
	//{
	//	return -1;
	//}

	//rtp_rtcp->GetEstimatedReceiveBandwidth(call->m_VideoChannelID, estimated_bandwidth);
	//rtp_rtcp->Release();
	return 0;
}

int ServiceCore::GetReceiveChannelRtcpStatistics(const char* callid, 
												_RtcpStatistics& basic_stats,
												__int64& rtt_ms)
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if (!call)
	{
		return -1;
	}

	//TODO:
	//ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);

	//if (!rtp_rtcp)
	//{
	//	return -1;
	//}

	//cloopenwebrtc::RtcpStatistics stats;
	//rtp_rtcp->GetReceiveChannelRtcpStatistics(call->m_VideoChannelID, stats, rtt_ms);

	//basic_stats.cumulative_lost = stats.cumulative_lost;
	//basic_stats.fraction_lost = stats.fraction_lost;
	//basic_stats.jitter = stats.jitter;
	//basic_stats.extended_max_sequence_number = stats.extended_max_sequence_number;

	//rtp_rtcp->Release();
	return 0;
}

int ServiceCore::GetSendChannelRtcpStatistics(const char* callid,
	_RtcpStatistics& basic_stats,
	__int64& rtt_ms)
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if (!call)
	{
		return -1;
	}

	//TODO:
	//ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);

	//if (!rtp_rtcp)
	//{
	//	return -1;
	//}

	//cloopenwebrtc::RtcpStatistics stats;
	//rtp_rtcp->GetSendChannelRtcpStatistics(call->m_VideoChannelID, stats, rtt_ms);

	//basic_stats.cumulative_lost = stats.cumulative_lost;
	//basic_stats.fraction_lost = stats.fraction_lost;
	//basic_stats.jitter = stats.jitter;
	//basic_stats.extended_max_sequence_number = stats.extended_max_sequence_number;

	//rtp_rtcp->Release();
	return 0;
}

int ServiceCore::GetRtpStatistics(const char* callid,
	_StreamDataCounters& sent,
	_StreamDataCounters& received)
{
	SerPhoneCall *call = serphone_core_get_current_call();
	if (!call)
	{
		return -1;
	}

	//TODO:
	//ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);

	//if (!rtp_rtcp)
	//{
	//	return -1;
	//}

	//cloopenwebrtc::StreamDataCounters sent_;
	//cloopenwebrtc::StreamDataCounters received_;
	//rtp_rtcp->GetRtpStatistics(call->m_VideoChannelID, sent_, received_);

	//sent.first_packet_time_ms = sent_.first_packet_time_ms;
	//sent.bytes = sent_.bytes;
	//sent.header_bytes = sent_.header_bytes;
	//sent.padding_bytes = sent_.padding_bytes;
	//sent.fec_packets = sent_.fec_packets;
	//sent.packets = sent_.packets;
	//sent.retransmitted_bytes = sent_.retransmitted_bytes;
	//sent.retransmitted_header_bytes = sent_.retransmitted_header_bytes;
	//sent.retransmitted_padding_bytes = sent_.retransmitted_padding_bytes;
	//sent.retransmitted_packets = sent_.retransmitted_packets;


	//received.first_packet_time_ms = received_.first_packet_time_ms;
	//received.bytes = received_.bytes;
	//received.header_bytes = received_.header_bytes;
	//received.padding_bytes = received_.padding_bytes;
	//received.fec_packets = received_.fec_packets;
	//received.packets = received_.packets;
	//received.retransmitted_bytes = received_.retransmitted_bytes;
	//received.retransmitted_header_bytes = received_.retransmitted_header_bytes;
	//received.retransmitted_padding_bytes = received_.retransmitted_padding_bytes;
	//received.retransmitted_packets = received_.retransmitted_packets;


	//rtp_rtcp->Release();	
	return 0;
}

//TODO:
//int ServiceCore::GetSendStats(const char* callid, VideoSendStream::Stats &sendStats)
//{
//	SerPhoneCall *call = serphone_core_get_current_call();
//	if (!call)
//	{
//		return -1;
//	}
//	sendStats = pSendStats_->GetStats();
//	return 0;
//}

void ServiceCore::serserphone_call_start_desktop_share(SerPhoneCall *call, const char *cname,bool_t all_inputs_muted)
{
#ifdef VIDEO_ENABLED
	if (m_SnapshotChannelID>=0) {
		stopVideoWithoutCall();
	}

	int used_pt=-1;
	/* look for savp stream first */
	const SalStreamDescription *stream = sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpSavp,SalVideo);
	const SalStreamDescription *local_stream = sal_media_description_find_stream(call->localdesc,
		SalProtoRtpSavp,SalVideo);

	/* no savp video stream, use avp */
	if (!stream)
		stream=sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpAvp,SalVideo);
	if (!local_stream)
		local_stream = sal_media_description_find_stream(call->localdesc,
		SalProtoRtpAvp,SalVideo);

	if (stream && stream->dir!=SalStreamInactive && stream->port!=0){
		call->video_profile=make_profile(call,call->resultdesc,stream,&used_pt);
		if (used_pt == -1){
			PrintConsole("No video stream accepted ?\n");
		}
		else {
			call->current_params.video_codec = rtp_profile_get_payload(call->video_profile, used_pt);
			PayloadType *p=call->current_params.video_codec;

			//TODO:
//			ViEBase* base = ViEBase::GetInterface(m_vie);
//			ViECodec* codec = ViECodec::GetInterface(m_vie);
//			ViENetwork *network = ViENetwork::GetInterface(m_vie);
//			ViERTP_RTCP *rtp_rtcp = ViERTP_RTCP::GetInterface(m_vie);
//			ViEImageProcess *image_processing = ViEImageProcess::GetInterface(m_vie);
//			ViEDesktopShare *vie_desktop_share = ViEDesktopShare::GetInterface(m_vie);
//			
//			//base->SetVoiceEngine(m_voe);
//
//			DesktopShareType desktopShareType=ShareScreen;
//			int desktopCaputreId;
//			int screen_width,screen_height;
//			vie_desktop_share->AllocateDesktopShareCapturer(desktopCaputreId, desktopShareType);
//			vie_desktop_share->ConnectDesktopCaptureDevice(desktopCaputreId, call->m_VideoChannelID);
//
//			vie_desktop_share->StartDesktopShareCapture(desktopCaputreId, m_sendVideoFps);
//			vie_desktop_share->GetDesktopShareCaptureRect(desktopCaputreId, screen_width, screen_height);
//			
//
//			if( videoWindow ) {
//#ifdef WIN32
//				RECT remoteRect;
//				::GetWindowRect((HWND)videoWindow, &remoteRect);
//				videoWindowSize = (remoteRect.right-remoteRect.left) * (remoteRect.bottom-remoteRect.top);
//#endif
//				ViERender* render =  ViERender::GetInterface(m_vie);
//				render->AddRenderer(call,call->m_VideoChannelID,videoWindow,2,0,0,1,1,/*sean 20130402*/return_video_width_height/*sean*/);
//				render->StartRender(call->m_VideoChannelID);
//				render->Release();
//			}
//
//			network->SetSendDestination(call->m_VideoChannelID,
//				stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr,stream->port);
//
//			VideoCodec codec_params;
//			bool codec_found = false;
//			for (int i = 0; i < codec->NumberOfCodecs(); i++) {
//				codec->GetCodec(i, codec_params);
//				if ( strcasecmp( codec_params.plName,p->mime_type ) == 0) {
//					codec_found = true;
//					codec_params.plType = used_pt;
//					break;
//				}
//			}
//
//			if (!codec_found) { 
//				PrintConsole("Can not find video codec %s \n", p->mime_type);
//			} else {
//				if(m_videoBitRates > 0 && m_videoBitRates > codec_params.minBitrate) {
//					codec_params.startBitrate = 300;
//				}
//
//				codec_params.width = screen_width;
//				codec_params.height = screen_height;
//				codec_params.maxFramerate = m_sendVideoFps;
//
//				PrintConsole("Video Codec is : playload type = %d, payload name is %s  bitrate=%d width=%d height=%d\n",
//					codec_params.plType, codec_params.plName, codec_params.startBitrate, codec_params.width, codec_params.height);
//
//				if (codec->SetSendCodec(call->m_VideoChannelID, codec_params) < 0)
//				{
//					PrintConsole("Error: SetSendCodec() fail!");
//				}
//
//				if (codec->SetReceiveCodec(call->m_VideoChannelID,codec_params) < 0)
//				{
//					PrintConsole("Error: SetReceiveCodec() fail!");
//				}
//
//				pSendStats_ = Serphone_set_video_send_statistics_proxy(call->m_VideoChannelID);
//				pReceiveStats_ = Serphone_set_video_receive_statistics_porxy(call->m_VideoChannelID);
//
//				if (!call->vie_observer)
//				{
//					call->vie_observer = new VieObserver(this);
//					codec->RegisterDecoderObserver(call->m_VideoChannelID, *call->vie_observer);
//				}	
//			}
//
//		
//	//		if(stream->nack_support)
//			{
//				PrintConsole("ENABLE_REMB_TMMBR_CONFIG: videoNackEnabled=%d  true, \n", videoNackEnabled);
//				rtp_rtcp->SetNACKStatus(call->m_VideoChannelID, videoNackEnabled);
//				rtp_rtcp->SetRTCPStatus(call->m_VideoChannelID, kRtcpNonCompound_RFC5506);
//			}
//			//else
//			{
//				PrintConsole("ENABLE_REMB_TMMBR_CONFIG: videoNackEnabled=%d  false, \n", videoNackEnabled);
//			}
//
//#ifdef ENABLE_REMB_TMMBR_CONFIG
//			PrintConsole("ENABLE_REMB_TMMBR_CONFIG: rembEnabled=%d, tmmbrEnabled=%d\n", rembEnabled, tmmbrEnabled);
//			rtp_rtcp->SetRembStatus(call->m_VideoChannelID, rembEnabled, rembEnabled);
//			rtp_rtcp->SetTMMBRStatus(call->m_VideoChannelID, tmmbrEnabled);
//
//			rtp_rtcp->SetSendAbsoluteSendTimeStatus(call->m_VideoChannelID, true, kRtpExtensionAbsoluteSendTime);
//			//rtp_rtcp->SetSendTimestampOffsetStatus(call->m_VideoChannelID, true, kRtpExtensionTransmissionTimeOffset);
//
//			rtp_rtcp->SetReceiveAbsoluteSendTimeStatus(call->m_VideoChannelID, true, kRtpExtensionAbsoluteSendTime);
//			//rtp_rtcp->SetReceiveTimestampOffsetStatus(call->m_VideoChannelID, true, kRtpExtensionTransmissionTimeOffset);
//#endif
//
//			if ( local_stream){
//				switch(local_stream->dir)
//				{
//				case SalStreamSendOnly:
//					base->StartSend(call->m_VideoChannelID);
//					break;
//				case SalStreamRecvOnly:
//					base->StartReceive(call->m_VideoChannelID);
//					break;
//				case SalStreamInactive:
//					break;
//				default:
//					base->StartReceive(call->m_VideoChannelID);
//					base->StartSend(call->m_VideoChannelID);
//					break;
//				}
//			}
//			else{
//				base->StartReceive(call->m_VideoChannelID);
//				base->StartSend(call->m_VideoChannelID);
//			}
//
//			
//
//
//#ifdef _WIN32           
//			if(call->record_voip && call->record_voip->isStartRecordMp4()) {
//				ViEFile *file = ViEFile::GetInterface(m_vie);
//				if(file) {
//					file->RegisterVideoFrameStorageCallBack(call->m_VideoChannelID, call->record_voip);
//					rtp_rtcp->RequestKeyFrame(call->m_VideoChannelID);
//				}
//				file->Release();
//			}
//#endif
//			//capture->Release();
//			network->Release();
//			codec->Release();
//			rtp_rtcp->Release();
//			base->Release();
//			image_processing->Release();
//			vie_desktop_share->Release();
//			call->current_params.in_conference=call->params.in_conference;
		}
	}
#endif
}

int ServiceCore::getShareScreenInfo(ScreenID **screenId, int captureId)
{
#ifdef VIDEO_ENABLED
	int num = ECMedia_get_screen_list(captureId, m_pScreenInfo);
	*screenId = m_pScreenInfo;

	return num;
#endif
	return -99;
}

int ServiceCore::getShareWindowInfo(WindowShare **windowInfo, int captureId)
{
#ifdef VIDEO_ENABLED
	int num = ECMedia_get_window_list(captureId, m_pWindowInfo);
	*windowInfo = m_pWindowInfo;

	return num;
#endif
	return -99;
}

int ServiceCore::Serphone_enable_opus_FEC(bool enable)
    {
        m_enable_fec = enable;
        return 0;
    }
int ServiceCore::Serphone_set_opus_packet_loss_rate(int rate)
{
        m_opus_packet_loss_rate = rate;
        return 0;
}

int ServiceCore::startRecord()
{
#if !defined(NO_VOIP_FUNCTION)
    return ECMedia_audio_start_record();
#endif
}

int ServiceCore::stopRecord()
{
#if !defined(NO_VOIP_FUNCTION)
    return ECMedia_audio_stop_record();
#endif
}

int ServiceCore::SetVideoKeepAlive(SerPhoneCall *call, bool enable, int interval)
{
#if !defined(NO_VOIP_FUNCTION)
#ifdef VIDEO_ENABLED
	if (!call)
		return -1;

	cloopenwebrtc::VideoCodec codec;
	ECMedia_get_send_codec_video(call->m_VideoChannelID, codec);
	return ECMedia_set_video_rtp_keepalive(call->m_VideoChannelID, enable, interval, codec.plType);
#endif
#endif
}
int ServiceCore::SetAudioKeepAlive(SerPhoneCall *call, bool enable, int interval)
{
#if !defined(NO_VOIP_FUNCTION)
	if (!call)
		return -1;
	cloopenwebrtc::CodecInst codec;
	ECMedia_get_send_codec_audio(call->m_AudioChannelID, codec);
	return ECMedia_set_audio_rtp_keepalive(call->m_AudioChannelID, enable, interval, codec.pltype);
#endif

}

//SendStatisticsProxy*  ServiceCore::Serphone_set_video_send_statistics_proxy(int video_channel)
//{
//#ifdef VIDEO_ENABLED
//	if (!m_vie)
//	{
//		return NULL;
//	}
//
//	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
//	ViECodec*	vie_codec = ViECodec::GetInterface(m_vie);
//	ViERTP_RTCP *vie_rtprtcp = ViERTP_RTCP::GetInterface(m_vie);
//
//	if (vie_base)
//	{
//		pSendStats_ = vie_base->GetSendStatisticsProxy(video_channel);
//		if (!pSendStats_)
//		{
//			return NULL;
//		}
//		vie_base->RegisterSendStatisticsProxy(video_channel, pSendStats_);
//		vie_base->Release();
//		if (vie_codec)
//		{
//			vie_codec->RegisterEncoderObserver(video_channel, *pSendStats_);
//			vie_codec->Release();
//		}
//		if (vie_rtprtcp)
//		{
//			vie_rtprtcp->RegisterSendChannelRtcpStatisticsCallback(video_channel, pSendStats_);
//			vie_rtprtcp->RegisterSendChannelRtpStatisticsCallback(video_channel, pSendStats_);
//			vie_rtprtcp->RegisterSendBitrateObserver(video_channel, pSendStats_);	
//			vie_rtprtcp->Release();
//		}
//		return pSendStats_;
//	}
//	
//	return NULL;
//
//#endif
//}
//
//ReceiveStatisticsProxy* ServiceCore::Serphone_set_video_receive_statistics_porxy(int video_channel)
//{
//#ifdef VIDEO_ENABLED
//	if (!m_vie)
//	{
//		return NULL;
//	}
//	ViEBase*	vie_base = ViEBase::GetInterface(m_vie);
//	ViECodec*	vie_codec = ViECodec::GetInterface(m_vie);
//	ViERTP_RTCP* vie_rtprtcp = ViERTP_RTCP::GetInterface(m_vie);
//
//	if (vie_base)
//	{
//		pReceiveStats_ = vie_base->GetReceiveStatisticsProxy(video_channel);
//		if (!pReceiveStats_)
//		{
//			return NULL;
//		}
//		vie_base->RegisterReceiveStatisticsProxy(video_channel, pReceiveStats_);
//		vie_base->Release();
//		if (vie_codec)
//		{
//			vie_codec->RegisterDecoderObserver(video_channel, *pReceiveStats_);
//			vie_codec->Release();
//		}
//		if (vie_rtprtcp)
//		{
//			vie_rtprtcp->RegisterReceiveChannelRtcpStatisticsCallback(video_channel, pReceiveStats_);
//			vie_rtprtcp->RegisterReceiveChannelRtpStatisticsCallback(video_channel, pReceiveStats_);
//			vie_rtprtcp->Release();
//		}
//
//		return pReceiveStats_;
//	}
//	return NULL;
//#endif
//
//}
