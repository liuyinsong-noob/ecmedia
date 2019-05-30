

#include "salpr.h"
#include "sometools.h"
#include "serphonecall.h"
#include "servicecore.h"
#include "lpconfig.h"
#include "CCPClient.h"
#include "sal_eXosip2.h"
#include "ECMedia.h"
extern CCallbackInterface g_cbInterface;

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details);
//#ifndef WIN32
//int printTime1();
//int printTime1()
//{
//    struct timeval system_time_high_res;
//    if (gettimeofday(&system_time_high_res, 0) == -1) {
//        return -1;
//    }
//    struct tm buffer;
//    const struct tm* system_time =
//    localtime_r(&system_time_high_res.tv_sec, &buffer);
//    
//    const uint32_t ms_time = system_time_high_res.tv_usec / 1000;
//    uint32_t prev_tickCount = 0;
//    
//    uint32_t dw_delta_time = ms_time - prev_tickCount;
//    if (prev_tickCount == 0) {
//        dw_delta_time = 0;
//    }
//    if (dw_delta_time > 0x0fffffff) {
//        // Either wraparound or data race.
//        dw_delta_time = 0;
//    }
//    if (dw_delta_time > 99999) {
//        dw_delta_time = 99999;
//    }
//    
//    printf("%02u:%02u:%02u:%3u | ", system_time->tm_hour,
//           system_time->tm_min, system_time->tm_sec, ms_time);
//    return 0;
//}
//#endif



bool_t media_parameters_changed(SerPhoneCall *call, SalMediaDescription *oldmd, SalMediaDescription *newmd){
//    sean update 20140116 for call pause resume begin
//	if (call->params.in_conference!=call->current_params.in_conference) return TRUE;
//	return !sal_media_description_equals(oldmd,newmd)  || call->up_bw!=call->core->serphone_core_get_upload_bandwidth();
    if (call->params.in_conference != call->current_params.in_conference) return SAL_MEDIA_DESCRIPTION_CHANGED;
	if (call->up_bw != call->core->serphone_core_get_upload_bandwidth()) return SAL_MEDIA_DESCRIPTION_CHANGED;
	return sal_media_description_equals(oldmd, newmd);
//    sean update 20140116 for call pause resume end
}

#if 0
static bool_t is_duplicate_call(LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to){
	MSList *elem;
	for(elem=lc->calls;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		if (linphone_address_weak_equal(call->log->from,from) &&
		    linphone_address_weak_equal(call->log->to, to)){
			return TRUE;
		}
	}
	return FALSE;
}
#endif


static void call_received(SalOp *h){

#if defined(NO_VOIP_FUNCTION)
	sal_call_decline(h,SalReasonNoVoip,NULL);
	sal_op_release(h);
	return;
#endif
    ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(h));
    
    WriteLogToFile("display name:%s",((SalOpBase *)h)->from);
    const char *fromBegin = strstr(((SalOpBase *)h)->from,":");
    const char *fromEnd = strstr(fromBegin, "@");
    char caller[512] = {0};
    memcpy(caller, fromBegin+1, fromEnd - fromBegin -1);
    lc->serphone_set_remote_sip(caller);
#ifdef HAIYUNTONG
    //create cipher:BK
    //判断是否含有ak
    if ((h->akey != NULL && !lc->serphone_haiyuntong_enabled()) || (h->akey == NULL && lc->serphone_haiyuntong_enabled())) {
        PrintConsole("[ERROR HAIYUNTONG] caller support haiyuntong while local does not support, decline\n");
        sal_call_decline(h, SalReasonMedia, NULL);
        sal_op_release(h);
        return;
    }
    if (lc->serphone_haiyuntong_enabled()) {
        int ret = lc->serphone_callee_create_bKey(h,caller, fromEnd-fromBegin-1);
        if (0 != ret) {
            PrintConsole("[ERROR HAIYUNTONG] Creating bkey failed,ret = %d, decline\n",ret);
            sal_call_decline(h, SalReasonMedia, NULL);
            sal_op_release(h);
            return;
        }
    }
#endif
    
    //设置一个回调，通知上层若是加密电话，则设置响应参数
    if (g_cbInterface.onEnableSrtp) {
        //将被叫号码取出来
        const char *endPos = strstr(((SalOpBase*)h)->from+1, "\"");
        int len = endPos - ((SalOpBase*)h)->from - 1;
        char *fromSip = new char[len+1];
        memcpy(fromSip, ((SalOpBase*)h)->from+1, len);
        fromSip[len] = '\0';
        g_cbInterface.onEnableSrtp(fromSip, false);
        delete []fromSip;
    }
	SerPhoneCall *call;
	const char *from,*to;
	SerphoneAddress *from_addr, *to_addr;
	bool_t prevent_colliding_calls=lp_config_get_int(lc->config,"sip","prevent_colliding_calls",TRUE);
	
	/* first check if we can answer successfully to this invite */
	if (lc->presence_mode==LinphoneStatusBusy ||
	    lc->presence_mode==LinphoneStatusOffline ||
	    lc->presence_mode==LinphoneStatusDoNotDisturb ||
	    lc->presence_mode==LinphoneStatusMoved){
		if (lc->presence_mode==LinphoneStatusBusy )
			sal_call_decline(h,SalReasonBusy,NULL);
		else if (lc->presence_mode==LinphoneStatusOffline)
			sal_call_decline(h,SalReasonTemporarilyUnavailable,NULL);
		else if (lc->presence_mode==LinphoneStatusDoNotDisturb)
			sal_call_decline(h,SalReasonTemporarilyUnavailable,NULL);
		else if (lc->alt_contact!=NULL && lc->presence_mode==LinphoneStatusMoved)
			sal_call_decline(h,SalReasonRedirect,lc->alt_contact);
		sal_op_release(h);
		return;
	}
	if (!lc->serphone_core_can_we_add_call()){/*busy*/
		sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		return;
	}
    
    ////If invite has no SDP
    //if (!((SalOpBase*)h)->remote_media) {
    //    PrintConsole("Remote media is empty, cancel here!\n");
    //    sal_call_decline(h, SalReasonMedia, NULL);
    //    sal_op_release(h);
    //    return;
    //}

    //If remote is SRTP and local attribute srtp_enable is false ,decline
    if (((SalOpBase*)h)->remote_media &&
		(!lc->srtp_enable && ((SalOpBase*)h)->remote_media->streams[0].proto == SalProtoRtpSavp)
        || (lc->srtp_enable && ((SalOpBase*)h)->remote_media->streams[0].proto == SalProtoRtpAvp)) {
        WriteLogToFile("Local media proto[%d] is different from that from remote[%d]\n", lc->srtp_enable?SalProtoRtpSavp:SalProtoRtpAvp, ((SalOpBase*)h)->remote_media->streams[0].proto);
        sal_call_decline(h, SalReasonMedia, NULL);
        sal_op_release(h);
        return;
    }
    
	from=sal_op_get_from(h);
	to=sal_op_get_to(h);
	from_addr=serphone_address_new(from);
	to_addr=serphone_address_new(to);
    
	if ((lc->already_a_call_with_remote_address(from_addr) && prevent_colliding_calls) || lc->already_a_call_pending()){
		WriteLogToFile("Receiving another call while one is ringing or initiated, refusing this one with busy message.\n");
		sal_call_decline(h,SalReasonBusy,NULL);
		sal_op_release(h);
		serphone_address_destroy(from_addr);
		serphone_address_destroy(to_addr);
		return;
	}

	call=lc->serphone_call_new_incoming(from_addr,to_addr,h);
	
	/* the call is acceptable so we can now add it to our list */
	lc->serphone_core_add_call(call);
	serphone_call_ref(call); /*prevent the call from being destroyed while we are notifying, if the user declines within the state callback */
    
    /*add begin------------------Sean20130622----------for video ice------------*/
	if ((lc->serphone_core_get_firewall_policy() == LinphonePolicyUseIce) && (call->ice_session != NULL)) {
		/* Defer ringing until the end of the ICE candidates gathering process. */
		WriteLogToFile("Defer ringing to gather ICE candidates");
		return;
	}
#ifdef BUILD_UPNP
	if ((lc->linphone_core_get_firewall_policy() == LinphonePolicyUseUpnp) && (call->upnp_session != NULL)) {
		/* Defer ringing until the end of the ICE candidates gathering process. */
		PrintConsole("Defer ringing to gather uPnP candidates");
		return;
	}
#endif //BUILD_UPNP
    /*add end--------------------Sean20130622----------for video ice------------*/
    
	lc->serphone_core_notify_incoming_call(call);
}

//caller side
static void call_ringing(SalOp *h){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(h));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(h);
	SalMediaDescription *md;
	
	if (call==NULL) return;
	
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Remote ringing."));
	//todaysean
    const char *tempRemote = sal_op_get_remote_sip(h);
    char remoteSip[512] = {0};
    const char *sipBegin = strstr(tempRemote, ":");
    const char *sipEnd = strstr(sipBegin, "@");
    memcpy(remoteSip, sipBegin+1, sipEnd-sipBegin-1);
#ifdef HAIYUNTONG
    if (lc->serphone_haiyuntong_enabled() && !lc->serphone_get_isAudioConf()) {
        int ret = lc->serphone_caller_180_183_transmitKeySet(h, remoteSip, strlen(remoteSip));
        if (0 != ret) {
            PrintConsole("[DEBUG HAIYUNTONG] No bkey failed, decline, h addr:%p\n", h);
            lc->serphone_core_abort_call(call,_("Incompatible, check codecs..."));
            return;
        }
    }
    
#endif
	md=sal_call_get_final_media_description(h);
	if (md==NULL){
		if ( lc->m_ringplay_flag ){
			lc->ring_stop();
			lc->m_ringplay_flag = false;
			lc->dmfs_playing_start_time = 0;
		}
		if ( lc->m_ringplay_flag ) return; //already ringing
		if (lc->vtable.display_status) 
			lc->vtable.display_status(lc,_("Remote ringing..."));
		lc->m_ringplay_flag = TRUE;
//        here stop playing prering
#ifdef WIN32
        lc->serphone_pre_after_ring_stop(true);
#endif
		lc->ring_start(lc->sound_conf.remote_ring,2000);
		serphone_call_set_state(call,LinphoneCallOutgoingRinging,"Remote ringing");
	}
	else{
		//accept early media 
		if ( call->m_audiostream_flag )
		{
			//streams already started 
			WriteLogToFile("Early media already started.\n");
			return;
		}
		if (lc->vtable.show) lc->vtable.show(lc);
		if (lc->vtable.display_status) 
			lc->vtable.display_status(lc,_("Early media."));
		serphone_call_set_state(call,LinphoneCallOutgoingEarlyMedia,"Early media");
		if ( lc->m_ringplay_flag ){
			lc->ring_stop();
			lc->m_ringplay_flag=FALSE;
			lc->dmfs_playing_start_time=0;
		}
		WriteLogToFile("Doing early media...\n");
		lc->serphone_core_update_streams(call,md);
	}
}

/*
 * could be reach :
 *  - when the call is accepted
 *  - when a request is accepted (pause, resume)
 */


static void call_accepted(SalOp *op){
    
//    printTime1();printf("seansean 200ok received\n");
    
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	SalMediaDescription *md;
	
	if (call==NULL){
		WriteLogToFile("No call to accept.\n");
		return ;
	}
	
    /* Handle remote ICE attributes if any. */
	if (call->ice_session != NULL) {
        lc->serphone_core_update_ice_from_remote_media_description(call, sal_call_get_remote_media_description(op));
	}
#ifdef HAIYUNTONG
    if (lc->serphone_haiyuntong_enabled() && lc->serphone_get_isAudioConf()) {
        
        lc->serphone_dump_conf_key(op);
        int ret = lc->serphone_audio_conf_key_set(op);
        if (0 != ret) {
            PrintConsole("[ERROR HAIYUNTONG] audio conf set key failed, decline\n");
            lc->serphone_core_abort_call(call,_("Incompatible, check codecs..."));
            return;
        }
    }
#endif
    
	md=sal_call_get_final_media_description(op);
    bool hasvideo = false;
    for ( int i = 0 ; i < md->nstreams ; i++ ) {
        if( md->streams[i].type == SalVideo && md->streams[i].rtp_port > 0) {
            hasvideo = true;
        }
    }
    call->params.has_video &= hasvideo;
	
    WriteLogToFile("call_accepted call->state=%d\n", call->state);
    
	if (call->state==LinphoneCallOutgoingProgress ||
		call->state==LinphoneCallOutgoingProceeding ||
	    call->state==LinphoneCallOutgoingRinging ||
	    call->state==LinphoneCallOutgoingEarlyMedia){
		serphone_call_set_state(call,LinphoneCallConnected,"Connected");
		if (call->referer) lc->serphone_core_notify_refer_state(call->referer,call);
	}
	if (md && !sal_media_description_empty(md)){
		if (sal_media_description_has_dir(md,SalStreamSendOnly) ||
		    sal_media_description_has_dir(md,SalStreamInactive)){
			if (lc->vtable.display_status){
				char *tmp=serphone_call_get_remote_address_as_string (call);
				char *msg=ms_strdup_printf(_("Call with %s is paused."),tmp);
				lc->vtable.display_status(lc,msg);
				ms_free((void **)&tmp);
                tmp = NULL;
				ms_free((void **)&msg);
                msg = NULL;
			}
			lc->serphone_core_update_streams (call,md);
			serphone_call_set_state(call,LinphoneCallPaused,"Call paused");
			if (call->refer_pending)
				lc->serphone_core_start_refered_call(call);
		}else if (sal_media_description_has_dir(md,SalStreamRecvOnly)){
			/*we are put on hold when the call is initially accepted */
			if (lc->vtable.display_status){
				char *tmp=serphone_call_get_remote_address_as_string (call);
				char *msg=ms_strdup_printf(_("Call answered by %s - on hold."),tmp);
				lc->vtable.display_status(lc,msg);
				ms_free((void **)&tmp);
                tmp = NULL;
				ms_free((void **)&msg);
                msg = NULL;
			}
			lc->serphone_core_update_streams (call,md);
			serphone_call_set_state(call,LinphoneCallPausedByRemote,"Call paused by remote");
		}else{
			if (call->state==LinphoneCallStreamsRunning){
				/*media was running before, the remote as acceted a call modification (that is
					a reinvite made by us. We must notify the application this reinvite was accepted*/
				serphone_call_set_state(call, LinphoneCallUpdated, "Call updated");
			}else{
				if (call->state==LinphoneCallResuming){
					if (lc->vtable.display_status){
						lc->vtable.display_status(lc,_("Call resumed."));
					}
                    serphone_call_set_state(call, LinphoneCallResumed, "Call resumed");
				}else{
					if (lc->vtable.display_status){
						char *tmp=serphone_call_get_remote_address_as_string (call);
						char *msg=ms_strdup_printf(_("Call answered by %s."),tmp);
						lc->vtable.display_status(lc,msg);
						ms_free((void **)&tmp);
                        tmp = NULL;
						ms_free((void **)&msg);
                        msg = NULL;
					}
				}
			}
			lc->serphone_core_restart_nack(call);
			lc->serphone_core_update_streams (call,md);
			if (!call->current_params.in_conference)
				lc->current_call=call;
			call->callConnected = true;
			serphone_call_set_state(call, LinphoneCallStreamsRunning, "Streams running");
            
//			requestVideo(g_currentCallId, 320, 240);
            requestVideo(call->_user_call_id, 320, 240);
            if (lc->serphone_get_isRefering()) {
                lc->serphone_set_isRefering(false);
                lc->serphone_core_transfer_call(call, lc->serphone_get_referTo(), 0);
            }
		}
	}else{
		/*send a bye*/
		WriteLogToFile("Incompatible SDP offer received in 200Ok, need to abort the call\n");
		lc->serphone_core_abort_call(call,_("Incompatible, check codecs..."));
	}
}
static void call_proceeding(SalOp *op) {
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	if (NULL == call)
		return;
    if(call->state != LinphoneCallStreamsRunning && call->state != LinphoneCallResuming && call->state != LinphoneCallPausing)
        serphone_call_set_state(call,LinphoneCallOutgoingProceeding,"Outgoing call in proceeding");
	return;
}

static void call_ack(SalOp *op){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	if (call==NULL){
		WriteLogToFile("No call to be ACK'd\n");
		return ;
	}
	if (call->media_pending){
		SalMediaDescription *md=sal_call_get_final_media_description(op);
		if (md && !sal_media_description_empty(md)){
			if (call->state==LinphoneCallStreamsRunning){
				/*media was running before, the remote as acceted a call modification (that is
					a reinvite made by us. We must notify the application this reinvite was accepted*/
				serphone_call_set_state(call, LinphoneCallUpdated, "Call updated");
			}
			lc->serphone_core_update_streams (call,md);
			call->callConnected = true;
			serphone_call_set_state (call,LinphoneCallStreamsRunning,"Connected (streams running)");
		}else{
			/*send a bye*/
			WriteLogToFile("Incompatible SDP response received in ACK, need to abort the call\n");
			lc->serphone_core_abort_call(call,"No codec intersection");
			return;
		}
	}
}

static void call_accept_update(ServiceCore *lc, SerPhoneCall *call){
	SalMediaDescription *md;
    
    SalMediaDescription *rmd=sal_call_get_remote_media_description(call->op);
	if ((rmd!=NULL) && (call->ice_session!=NULL)) {
		lc->serphone_core_update_ice_from_remote_media_description(call,rmd);
		lc->serphone_core_update_local_media_description_from_ice(call->localdesc,call->ice_session);
	}
    
    lc->serphone_call_update_remote_session_id_and_ver(call);
	sal_call_accept(call->op);
	md=sal_call_get_final_media_description(call->op);
	if (md && !sal_media_description_empty(md))
		lc->serphone_core_update_streams(call,md);
}

static void call_resumed(ServiceCore *lc, SerPhoneCall *call){
	call_accept_update(lc,call);
	if(lc->vtable.display_status)
		lc->vtable.display_status(lc,_("We have been resumed."));
	serphone_call_set_state(call,LinphoneCallStreamsRunning,"Connected (streams running)");
	serphone_call_set_transfer_state(call, LinphoneCallIdle);
}

static void call_paused_by_remote(ServiceCore *lc, SerPhoneCall *call){
	call_accept_update(lc,call);
	/* we are being paused */
	if(lc->vtable.display_status)
		lc->vtable.display_status(lc,_("We are paused by other party."));
	serphone_call_set_state (call,LinphoneCallPausedByRemote,"Call paused by remote");
}

static void call_updated_by_remote(ServiceCore *lc, SerPhoneCall *call){
	if(lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Call is updated by remote."));
	call->defer_update=FALSE;
    
    
	SalMediaDescription *md=sal_call_get_remote_media_description(call->op);
	
    bool hasvideo = false;
    for ( int i = 0 ; i < md->nstreams ; i++ ) {
        if( md->streams[i].type == SalVideo && md->streams[i].rtp_port > 0) {
            hasvideo = true;
        }
    }
    call->remote_params.has_video = hasvideo;
	serphone_call_set_state(call, LinphoneCallUpdatedByRemote,"Call updated by remote");
	if (call->defer_update==FALSE){
		lc->serphone_core_accept_call_update(call,NULL);
	}
}

/* this callback is called when an incoming re-INVITE modifies the session*/
static void call_updating(SalOp *op){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	SalMediaDescription *rmd=sal_call_get_remote_media_description(op);

	if (rmd==NULL){
		/* case of a reINVITE without SDP */
		call_accept_update(lc,call);
		call->media_pending=TRUE;
		return;
	}

	switch(call->state){
		case LinphoneCallPausedByRemote:
			if (sal_media_description_has_dir(rmd,SalStreamSendRecv) || sal_media_description_has_dir(rmd,SalStreamRecvOnly)){
				call_resumed(lc,call);
			}
		break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallConnected:
			if (sal_media_description_has_dir(rmd,SalStreamSendOnly) || sal_media_description_has_dir(rmd,SalStreamInactive)){
				call_paused_by_remote(lc,call);
			}else{
				call_updated_by_remote(lc,call);
			}
		break;
		default:
			call_accept_update(lc,call);
	}
}

static void call_terminated(SalOp *op, const char *from){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);

	if (call==NULL) return;
	
	if (serphone_call_get_state(call)==LinphoneCallEnd || serphone_call_get_state(call)==LinphoneCallError){
		WriteLogToFile("call_terminated: ignoring.\n");
		return;
	}
	WriteLogToFile("Current call terminated...\n");
	//we stop the call only if we have this current call or if we are in call
	if ( lc->m_ringplay_flag && ( (ms_list_size(lc->calls)  == 1) || lc->serphone_core_in_call() )) {
		lc->ring_stop();
		lc->m_ringplay_flag = FALSE;
		lc->dmfs_playing_start_time = 0;
	}

	//mute change the system device status and will stay on the next startup
	//so resume when call end
	lc->serphone_set_mute_status(false);
	lc->serphone_set_speaker_mute_status(false);

	lc->serphone_call_stop_media_streams(call);
	if (lc->vtable.show!=NULL)
		lc->vtable.show(lc);
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,_("Call terminated."));

	serphone_call_set_state(call, LinphoneCallEnd,"Call ended");
}

static void call_failure(SalOp *op, SalError error, SalReason sr, const char *details, int code){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	const char *msg430=_("User does not support voip.");
	const char *msg486=_("User is busy.");
	const char *msg480=_("User is temporarily unavailable.");
	/*char *retrymsg=_("%s. Retry after %i minute(s).");*/
	const char *msg600=_("User does not want to be disturbed.");
	const char *msg603=_("Call declined.");
	const char *msg=details;
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);

	if (call==NULL){
		WriteLogToFile("Call faillure reported on already cleaned call ?\n");
		return ;
	}
    
	if (lc->vtable.show) lc->vtable.show(lc);
    
    int detailsReason = 0;
    if(details)
    {
        char *causeStr = (char*)strstr(details, "cause=");
        if(causeStr)
        {
            char *endCause = strstr(causeStr, ";");
            if(endCause)
                *endCause = 0;
            detailsReason = atoi(causeStr+6);
        }
    }


	if (error==SalErrorNoResponse){
		call->reason = SerphoneReasonNoResponse;
		msg=_("No response.");
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc,msg);
	}else if (error==SalErrorProtocol){
		msg=details ? details : _("Protocol error.");
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc, msg);
	}else if (error==SalErrorFailure){
		switch(sr){
			case SalReasonDeclined:
				msg=msg603;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg603);
			break;
            case SalReasonNoVoip:
				call->reason = SerphoneReasonNoVoip;
				msg=msg430;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg430);
				break;
			case SalReasonBusy:
				call->reason = SerphoneReasonBusy;
				msg=msg486;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg486);
			break;
			case SalReasonRedirect:
				msg=_("Redirected");
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg);
			break;
			case SalReasonTemporarilyUnavailable:
				msg=msg480;
				if( time(NULL)-call->start_time > 50 )
#ifdef OLDERRORCODE
					call->reason = SerphoneReasonCallMissed;
#else
                    call->reason = SerphoneReasonTimeout;
#endif
				else
                {
#ifdef OLDERRORCODE
					call->reason = (SerphoneReason) 480;
#else
                    call->reason = (SerphoneReason) 175480;
#endif
                }
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg480);
			break;
			case SalReasonNotFound:
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg);
			break;
			case SalReasonDoNotDisturb:
				msg=msg600;
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg600);
			break;
			case SalReasonMedia:
            {
#ifdef OLDERRORCODE
                call->reason =(SerphoneReason) code;
#else
                call->reason =(SerphoneReason) (175000+code);
#endif
			//media_encryption_mandatory
				/*   ���ܵ�ý�����ݲ�������
				if (call->params.media_encryption == LinphoneMediaEncryptionSRTP && 
					!linphone_core_is_media_encryption_mandatory(lc)) {
					int i;
					PrintConsole("Outgoing call failed with SRTP (SAVP) enabled - retrying with AVP\n");
					lc->serphone_call_stop_media_streams(call);
					if (call->state==LinphoneCallOutgoingInit || call->state==LinphoneCallOutgoingProgress){
						// clear SRTP local params 
						call->params.media_encryption = LinphoneMediaEncryptionNone;
						for(i=0; i<call->localdesc->nstreams; i++) {
							call->localdesc->streams[i].proto = SalProtoRtpAvp;
							memset(call->localdesc->streams[i].crypto, 0, sizeof(call->localdesc->streams[i].crypto));
						}
						lc->serphone_core_start_invite(call, NULL);
					}
					return;
				}
				*/
				msg=_("Incompatible media parameters.");
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,msg);
            }
			break;
			default:
            {
#ifdef OLDERRORCODE
				call->reason =(SerphoneReason)(code);
#else
                call->reason =(SerphoneReason)(175000+code);
#endif
				if (lc->vtable.display_status)
					lc->vtable.display_status(lc,_("Call failed."));
            }
		}
	}

	if ( lc->m_ringplay_flag ){
		lc->ring_stop();
		lc->m_ringplay_flag=FALSE;
		lc->dmfs_playing_start_time=0;
	}
	lc->serphone_call_stop_media_streams (call);
	if (call->referer && serphone_call_get_state(call->referer)==LinphoneCallPaused && call->referer->was_automatically_paused){
		/*resume to the call that send us the refer automatically*/
		lc->serphone_core_resume_call(call->referer);
	}
    
    if( detailsReason >= 700)
    {
#ifdef OLDERRORCODE
        call->reason =(SerphoneReason)(detailsReason);
#else
        call->reason =(SerphoneReason)(175000+detailsReason);
#endif
        serphone_call_set_state(call,LinphoneCallError,msg);
        return;
    }
    
	if (sr == SalReasonDeclined) {
		if( time(NULL)-call->start_time > 50) {
#ifdef OLDERRORCODE
			call->reason=SerphoneReasonCallMissed;
#else
            call->reason=SerphoneReasonTimeout;
#endif
			serphone_call_set_state(call,LinphoneCallEnd,"Call not answered.");
		}
		else {
			call->reason=SerphoneReasonDeclined;
			serphone_call_set_state(call,LinphoneCallEnd,"Call declined.");
		}
		
	} else if (sr == SalReasonNotFound) {
		call->reason=SerphoneReasonNotFound;
		serphone_call_set_state(call,LinphoneCallError,"User not found.");
	}else {
		serphone_call_set_state(call,LinphoneCallError,msg);
	}
}

static void call_released(SalOp *op){
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	if (call!=NULL){
		serphone_call_set_state(call,LinphoneCallReleased,"Call released");
	}else WriteLogToFile("call_released() for already destroyed call ?\n");
}


static void auth_requested(SalOp *h, const char *realm, const char *username){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(h));
	lc->serphone_core_update_auth_info(realm, username);
	SerphoneAuthInfo *ai=(SerphoneAuthInfo*)lc->serphone_core_find_auth_info(realm,username);
	SerPhoneCall *call=is_a_linphone_call(sal_op_get_user_pointer(h));

	if (call && call->ping_op==h){
		/*don't request authentication for ping requests. Their purpose is just to get any
		 * answer to get the Via's received and rport parameters.
		 */
		WriteLogToFile("auth_requested(): ignored for ping request.\n");
		return;
	}
	
	if (ai && ai->works==FALSE && ai->usecount>=3){
		/*case we tried 3 times to authenticate, without success */
		/*Better is to stop (implemeted below in else statement), and retry later*/
		if (ms_time(NULL)-ai->last_use_time>30){
			ai->usecount=0; /*so that we can allow to retry */
		}
	}
	
	if (ai && (ai->works || ai->usecount<3)){
		SalAuthInfo sai;
		sai.username=ai->username;
		sai.userid=ai->userid;
		sai.realm=ai->realm;
		sai.password=ai->passwd;
		sal_op_authenticate(h,&sai);
		ai->usecount++;
		ai->last_use_time=ms_time(NULL);
	}else{
		if (ai && ai->works==FALSE) {
			sal_op_cancel_authentication(h);
		} 
		if (lc->vtable.auth_info_requested)
			lc->vtable.auth_info_requested(lc,realm,username);
	}
}

static void auth_success(SalOp *h, const char *realm, const char *username){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(h));
	SerphoneAuthInfo *ai=(SerphoneAuthInfo*)lc->serphone_core_find_auth_info(realm,username);
	if (ai){
		ai->works=TRUE;
	}
}

static void register_success(SalOp *op, bool_t registered){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)sal_op_get_user_pointer(op);
	char *msg;
	const char *contact=sal_op_get_contact(op);
	if( registered && contact) {
		lc->serphone_core_set_primary_contact(contact);
	}

    if (!registered) {
        MSList *elem = lc->serphone_core_get_remove_config_list();
        for(;elem!=NULL;elem=ms_list_next(elem)){
            SerphoneProxyConfig *config=(SerphoneProxyConfig*)(elem->data);
            if(cfg == config) break;
        }
        
        if( ! elem)
        {
            WriteLogToFile("This Proxy config:%d is already destroyed.\n", cfg);
            return;
        }
    }
	

	
//	if (cfg->deletion_date!=0){
//		PrintConsole("Registration success for removed proxy config, ignored\n");
//		return;
//	}
	cfg->serphone_proxy_config_set_error(SerphoneReasonNone);
	cfg->serphone_proxy_config_set_state(registered ? LinphoneRegistrationOk : LinphoneRegistrationCleared ,
	                                registered ? "Registration sucessful" : "Unregistration done");
	if (lc->vtable.display_status){
		if (registered) msg=ms_strdup_printf(_("Registration on %s successful."),sal_op_get_proxy(op));
		else msg=ms_strdup_printf(_("Unregistration on %s done."),sal_op_get_proxy(op));
		lc->vtable.display_status(lc,msg);
		ms_free((void **)&msg);
        msg = NULL;
	}
	
}

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerphoneProxyConfig *cfg=(SerphoneProxyConfig*)sal_op_get_user_pointer(op);

	if (cfg==NULL){
		WriteLogToFile("Registration failed for unknown proxy config.\n");
		return ;
	}
	if (cfg->deletion_date!=0){
		WriteLogToFile("Registration failed for removed proxy config, ignored\n");
		return;
	}
	if (details==NULL)
		details=_("no response timeout");
	
	if (lc->vtable.display_status) {
		char *msg=ser_strdup_printf(_("Registration on %s failed: %s"),sal_op_get_proxy(op),details  );
		lc->vtable.display_status(lc,msg);
		ms_free((void **)&msg);
        msg = NULL;
	}
	if (error== SalErrorFailure && reason == SalReasonForbidden) {
		cfg->serphone_proxy_config_set_error( SerphoneReasonBadCredentials);
	} else if (error == SalErrorNoResponse) {
		cfg->serphone_proxy_config_set_error( SerphoneReasonNoResponse);
	}
	cfg->serphone_proxy_config_set_state(LinphoneRegistrationFailed,details);
	if (error== SalErrorFailure && reason == SalReasonForbidden) {
		const char *realm=NULL,*username=NULL;
		if (sal_op_get_auth_requested(op,&realm,&username)==0){
			if (lc->vtable.auth_info_requested)
				lc->vtable.auth_info_requested(lc,realm,username);
		}
        
        ms_list_for_each(lc->sip_conf.proxies,(void (*)(void**)) serphone_proxy_config_destroy);
        ms_list_free(lc->sip_conf.proxies);
        lc->sip_conf.proxies=NULL;
        lc->serphone_core_set_default_proxy(NULL);
	}
    WriteLogToFile("Registration failed\n");
//    ms_list_for_each(lc->sip_conf.proxies,(void (*)(void*)) serphone_proxy_config_destroy);
//    ms_list_free(lc->sip_conf.proxies);
//	lc->sip_conf.proxies=NULL;
//    lc->serphone_core_set_default_proxy(NULL);
}

static void vfu_request(SalOp *op){
#ifdef VIDEO_ENABLED
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer (op);
	if (call==NULL){
		WriteLogToFile("VFU request but no call !\n");
		return ;
	}
//Sean add begin 20131022 for video fast update in video conference
	/*if (call->m_VideoChannelID)
		video_stream_send_vfu(call->videostream);*/
    if (call->m_VideoChannelID >= 0) {
        ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
        lc->serphone_send_key_frame(call);
    }
//Sean add end 20131022 for video fast update in video conference
#endif
}

static void dtmf_received(SalOp *op, char dtmf){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	if (lc->vtable.dtmf_received != NULL)
		lc->vtable.dtmf_received(lc, call->_user_call_id, dtmf);
}

static void refer_received(Sal *sal, SalOp *op, const char *referto){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal);
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer(op);
	if (call){
		if (call->refer_to!=NULL){
			ms_free((void **)&(call->refer_to));
		}
		call->refer_to=ms_strdup(referto);
		call->refer_pending=TRUE;
		serphone_call_set_state(call,LinphoneCallRefered,"Refered");
		if (lc->vtable.display_status){
			char *msg=ms_strdup_printf(_("We are transferred to %s"),referto);
			lc->vtable.display_status(lc,msg);
			ms_free((void **)&msg);
            msg = NULL;
		}
		if (call->state!=LinphoneCallPaused){
			WriteLogToFile("Automatically pausing current call to accept transfer.\n");
			lc->serphone_core_pause_call(call);
			call->was_automatically_paused=TRUE;
			/*then we will start the refered when the pause is accepted, in order to serialize transactions within the dialog.
			 * Indeed we need to avoid to send a NOTIFY to inform about of state of the refered call while the pause isn't completed.
			**/
		}else lc->serphone_core_start_refered_call(call);
	}else if (lc->vtable.refer_received){
		lc->vtable.refer_received(lc,referto);
	}
}

static void text_received(Sal *sal, const char *from, const char *to, const char *msgid, const char *msg, const char *date){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal);
	lc->serphone_core_text_received(from,to,msgid,msg,date);
}

static void text_send_report(Sal *sal, const char *msgid, const char *date, int sipcode){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal);
	if(lc->vtable.text_send_report)
		lc->vtable.text_send_report(lc,msgid,date,sipcode);
}

static void notify(SalOp *op, const char *from, const char *msg){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*)sal_op_get_user_pointer (op);
	WriteLogToFile("get a %s notify from %s\n",msg,from);
	if(lc->vtable.notify_recv)
		lc->vtable.notify_recv(lc,call,from,msg);
}

static void notify_presence(SalOp *op, SalSubscribeStatus ss, SalPresenceStatus status, const char *msg){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	lc->serphone_notify_recv(op,ss,status);
}

static void subscribe_received(SalOp *op, const char *from){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	lc->serphone_subscription_new(op,from);
}

static void subscribe_closed(SalOp *op, const char *from){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	lc->serphone_subscription_closed(op);
}

static void ping_reply(SalOp *op){
	SerPhoneCall *call=(SerPhoneCall*) sal_op_get_user_pointer(op);
	WriteLogToFile("ping reply !\n");
	if (call){
		if (call->state==LinphoneCallOutgoingInit){
			call->core->serphone_core_start_invite(call,NULL);
		}
	}
	else
	{
		WriteLogToFile("ping reply without call attached...\n");
	}
}

static void option_reply(SalOp *op, int status){
	int *account_status=(int*) sal_op_get_user_pointer(op);
	WriteLogToFile("option reply status=%d !\n", status);
	switch(status) {
		case 200:
			*account_status = account_Status_Online;
            break;
		case 404:
			*account_status = account_Status_Offline;
            break;
		case 408:
			*account_status = account_Status_TimeOut;
            break;
		case 431:
			*account_status = account_Status_NotExist;
            break;
		default:
			*account_status = account_Status_NotExist;
    }
}

static void notify_refer(SalOp *op, SalReferStatus status){
	ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal_op_get_sal(op));
	SerPhoneCall *call=(SerPhoneCall*) sal_op_get_user_pointer(op);
	SerphoneCallState cstate;
	if (call==NULL) {
		WriteLogToFile("Receiving notify_refer for unknown call.\n");
		return ;
	}
	switch(status){
		case SalReferTrying:
			cstate=LinphoneCallOutgoingProceeding;
		break;
		case SalReferSuccess:
			cstate=LinphoneCallConnected;
		break;
		case SalReferFailed:
			cstate=LinphoneCallError;
		break;
		default:
			cstate=LinphoneCallError;
	}
	serphone_call_set_transfer_state(call, cstate);
	if (cstate==LinphoneCallConnected){
		/*automatically terminate the call as the transfer is complete.*/
		lc->serphone_core_terminate_call(call);
	}
}

static void stun_packet(Sal *sal, const char *call_id, OrtpEvent *stun_packet, bool_t is_video)
{
    WriteLogToFile("ICE:stun_packet\n");
                 
    ServiceCore *lc=(ServiceCore *)sal_get_user_pointer(sal);
    
    SerPhoneCall *call = NULL;
    
    if(is_video) {
        call = lc->serphone_core_find_call_by_user_cid(call_id);
    }
    else {
        call = lc->serphone_core_find_call_by_user_cid(call_id);
    }
    if( !call ) {
        WriteLogToFile("ICE: stun_packet can't find call by channelID=%s\n", call_id);
        return;
    }
    
    lc->handle_ice_events(call, stun_packet, lc, is_video);
}

SalCallbacks serphone_sal_callbacks={
	call_received,
	call_ringing,
	call_accepted,
	call_proceeding,
	call_ack,
	call_updating,
	call_terminated,
	call_failure,
	call_released,
	auth_requested,
	auth_success,
	register_success,
	register_failure,
	vfu_request,
	dtmf_received,
	refer_received,
	text_received,
	text_send_report,
	notify,
	notify_presence,
	notify_refer,
	subscribe_received,
	subscribe_closed,
	ping_reply,
    stun_packet,
	option_reply
};


